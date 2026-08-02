#include <allocator/pmm.h>
#include <utils/memory.h>

static uintptr_t s_freelistHead;
static uintptr_t s_freelistTail;
static size_t s_freelistSize;

static inline int div_ceil(int a, int b) { return (a + b - 1) / b; }
static inline int div_floor(int a, int b) { return a / b; /* duh */ }

// this bitmap should in theory help us detect double frees
static uint8_t *s_bitmap;
static size_t s_bitmapFrames;

static inline size_t frame_of(uintptr_t phys) { return phys / PAGE_SIZE; }

static inline int bitmap_test(size_t frame) {
  return (s_bitmap[frame / 8] >> (frame % 8)) & 1;
}
static inline void bitmap_set(size_t frame) {
  s_bitmap[frame / 8] |= (uint8_t)(1u << (frame % 8));
}
static inline void bitmap_clear(size_t frame) {
  s_bitmap[frame / 8] &= (uint8_t)~(1u << (frame % 8));
}

static void bitmap_mark_free(uintptr_t phys) {
  size_t frame = frame_of(phys);
  kassert(frame < s_bitmapFrames && "Frame out of bitmap range");
  kassert(!bitmap_test(frame) && "Double free detected");
  bitmap_set(frame);
}

static void bitmap_mark_used(uintptr_t phys) {
  size_t frame = frame_of(phys);
  kassert(frame < s_bitmapFrames && "Frame out of bitmap range");
  kassert(bitmap_test(frame) && "PMM allocated a frame not marked free");
  bitmap_clear(frame);
}

static void bitmap_bootstrap(size_t bytes) {
  size_t needed = div_ceil(bytes, PAGE_SIZE) * PAGE_SIZE;

  for (size_t i = 0; i < memmap_request.response->entry_count; i++) {
    struct limine_memmap_entry *entry = memmap_request.response->entries[i];
    if (entry->type != LIMINE_MEMMAP_USABLE)
      continue;

    if (entry->length < needed)
      continue;

    s_bitmap = (uint8_t *)p2v(entry->base);

    // should be safe to remove in place,
    // this is needed so bitmap regions
    // arent claimed by the pmm
    entry->base += needed;
    entry->length -= needed;

    for (size_t b = 0; b < bytes; b++)
      s_bitmap[b] = 0;
    return;
  }

  kpanic("No usable region large enough for PMM bitmap");
}

static void bitmap_init() {
  klog_info("Initializing PMM bitmap for double frees");

  uintptr_t highest = 0;
  for (size_t i = 0; i < memmap_request.response->entry_count; i++) {
    struct limine_memmap_entry *entry = memmap_request.response->entries[i];
    if (entry->type != LIMINE_MEMMAP_USABLE)
      continue;

    uintptr_t end = entry->base + entry->length;
    if (end > highest)
      highest = end;
  }

  s_bitmapFrames = div_ceil(highest, PAGE_SIZE);

  size_t bytes = div_ceil(s_bitmapFrames, 8);
  bitmap_bootstrap(bytes);

  klog_ok("PMM bitmap initialized!");
}

static void pmm_claim_pages(pmm_filter_function filter) {
  for (size_t i = 0; i < memmap_request.response->entry_count; i++) {
    struct limine_memmap_entry *entry = memmap_request.response->entries[i];
    if (filter(entry) != 0)
      continue;

    kassert(entry && "Bad limine memory map entry");

    // get the amount of pages possible for this region
    // if there is 4097 bytes then only 1 page is possible,
    // so we use div floor
    size_t pagesPossible = div_floor(entry->length, PAGE_SIZE);

    for (size_t j = 0; j < pagesPossible; j++) {
      size_t pageStart = entry->base + j * PAGE_SIZE;

      pmm_node_t *node = p2v(pageStart);
      node->next = 0;

      bitmap_mark_free(pageStart);
      // add this to the freelist, it is possible
      // that the freelist head may be 0
      if (s_freelistHead == 0) {
        s_freelistHead = pageStart;
      } else {
        ((pmm_node_t *)p2v(s_freelistTail))->next = pageStart;
      }

      s_freelistTail = pageStart;
      s_freelistSize++;
    }
  }
}

static int default_pmm_filter(struct limine_memmap_entry *entry) {
  if (entry->type == LIMINE_MEMMAP_USABLE)
    return 0;
  else
    return -1;
}

void pmm_init() {
  s_bitmap = NULL;
  s_bitmapFrames = 0;
  s_freelistHead = 0;
  s_freelistSize = 0;
  s_freelistTail = 0;

  klog_info("Initializing PMM...");
  bitmap_init();
  pmm_claim_pages(default_pmm_filter);

  klog_ok("PMM Initialized, +%llu new pages", s_freelistSize);
}

uintptr_t pmm_get_freelist_head() { return s_freelistHead; }

uintptr_t pmm_alloc() {
  // returns s_freelistHead
  uintptr_t frame = s_freelistHead;

  // we are out of physical mem
  if (!frame) {
    s_freelistTail = 0;
    return 0;
  }

  pmm_node_t *headVirt = (pmm_node_t *)p2v(s_freelistHead);
  s_freelistHead = (uintptr_t)headVirt->next;
  s_freelistSize--;
  bitmap_mark_used(frame);
  return frame;
}

void pmm_free(uintptr_t at) {
  bitmap_mark_free(at);

  // replaces s_freelistHead and pushes it forward
  pmm_node_t *pageVirt = (pmm_node_t *)p2v(at);
  pageVirt->next = s_freelistHead;
  s_freelistSize++;
  s_freelistHead = at;
}

static int bootloader_pmm_filter(struct limine_memmap_entry *entry) {
  if (entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE)
    return 0;
  else
    return -1;
}

static int acpi_pmm_filter(struct limine_memmap_entry *entry) {
  if (entry->type == LIMINE_MEMMAP_ACPI_RECLAIMABLE)
    return 0;
  else
    return -1;
}

void pmm_reclaim_bootloader_pages() {
  uintptr_t was = s_freelistSize;

  klog_info("Reclaiming bootloader pages");

  pmm_claim_pages(bootloader_pmm_filter);

  klog_ok("+%llu new pages", s_freelistSize - was);
}

void pmm_reclaim_acpi_pages() {
  uintptr_t was = s_freelistSize = 0;
  klog_info("Reclaiming ACPI pages");

  pmm_claim_pages(acpi_pmm_filter);

  klog_ok("+%llu new pages", s_freelistSize - was);
}

uintptr_t pmm_get_pages_count() { return s_freelistSize; }
