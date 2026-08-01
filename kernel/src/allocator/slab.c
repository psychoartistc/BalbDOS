#include <allocator/slab.h>

static kmalloc_bucket_t s_buckets[KMALLOC_NUM_BUCKETS];

void allocator_init(void) {
  for (size_t i = 0; i < KMALLOC_NUM_BUCKETS; i++) {
    s_buckets[i].chunk_size = s_bucket_sizes[i];
    s_buckets[i].free_list = NULL;
  }
  klog_ok("Slab allocator initialized with %d buckets", KMALLOC_NUM_BUCKETS);
}

static int find_bucket(size_t size) {
  for (int i = 0; i < KMALLOC_NUM_BUCKETS; i++) {
    if (size <= s_bucket_sizes[i])
      return i;
  }

  // we dont have this big of a bucket
  // go for vmm_alloc
  return -1;
}

// grows a bucket by mapping one fresh page and slicing it into slots
// [header (16B)][chunk_size bytes of data] threaded into the free list
static bool bucket_grow(kmalloc_bucket_t *bucket) {
  size_t slot_size = KMALLOC_HEADER_SIZE + bucket->chunk_size;
  size_t slots_per_page = PAGE_SIZE / slot_size;
  kassert(slots_per_page > 0 && "Bucket chunk larger than a page");

  void *page = vmm_alloc(PAGE_SIZE, VMM_WRITABLE | VMM_NX);
  if (!page)
    return false;

  uint8_t *base = (uint8_t *)page;
  for (size_t i = 0; i < slots_per_page; i++) {
    uint8_t *slot = base + i * slot_size;
    kmalloc_header_t *hdr = (kmalloc_header_t *)slot;
    void *data = slot + KMALLOC_HEADER_SIZE;

    hdr->magic = KMALLOC_MAGIC_FREE;

    *(void **)data = bucket->free_list;
    bucket->free_list = data;
  }
  return true;
}

void *kmalloc(size_t size) {
  if (size == 0)
    return NULL;

  int idx = find_bucket(size);

  if (idx < 0) {
    size_t total = KMALLOC_HEADER_SIZE + size;
    size_t pages = (total + PAGE_SIZE - 1) / PAGE_SIZE;
    void *mem = vmm_alloc(pages * PAGE_SIZE, VMM_WRITABLE | VMM_NX);
    if (!mem)
      return NULL;

    kmalloc_header_t *hdr = (kmalloc_header_t *)mem;
    hdr->magic = KMALLOC_MAGIC_RAW;
    hdr->bucket_index = (uint32_t)~0u;
    hdr->size = pages * PAGE_SIZE;
    return (uint8_t *)mem + KMALLOC_HEADER_SIZE;
  }

  kmalloc_bucket_t *bucket = &s_buckets[idx];
  if (!bucket->free_list) {
    if (!bucket_grow(bucket))
      return NULL;
  }

  void *data = bucket->free_list;
  bucket->free_list = *(void **)data;

  kmalloc_header_t *hdr =
      (kmalloc_header_t *)((uint8_t *)data - KMALLOC_HEADER_SIZE);
  kassert(hdr->magic == KMALLOC_MAGIC_FREE &&
          "Corrupted free list or bad magic on alloc");
  hdr->magic = KMALLOC_MAGIC_USED;
  hdr->bucket_index = (uint32_t)idx;

  return data;
}

void kfree(void *ptr) {
  if (!ptr)
    return;

  kmalloc_header_t *hdr =
      (kmalloc_header_t *)((uint8_t *)ptr - KMALLOC_HEADER_SIZE);

  if (hdr->magic == KMALLOC_MAGIC_FREE)
    kpanic("Double free detected at %p", ptr);

  if (hdr->magic == KMALLOC_MAGIC_RAW) {
    vmm_free(hdr, hdr->size);
    return;
  }

  kassert(hdr->magic == KMALLOC_MAGIC_USED &&
          "Bad kmalloc header magic, corrupted or invalid heap pointer");
  kassert(hdr->bucket_index < KMALLOC_NUM_BUCKETS &&
          "Bad bucket index corrupted heap pointer");

  kmalloc_bucket_t *bucket = &s_buckets[hdr->bucket_index];
  hdr->magic = KMALLOC_MAGIC_FREE;

  *(void **)ptr = bucket->free_list;
  bucket->free_list = ptr;
}
