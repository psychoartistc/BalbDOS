#include <allocator/vmm.h>
#include <limine/requests.h>
#include <panic.h>
#include <utils/kassert.h>
#include <utils/printk.h>

static inline size_t div_ceil(size_t a, size_t b) { return (a + b - 1) / b; }

vmm_pagemap_t g_kernel_pagemap;
static uintptr_t s_heap_bump = VMM_HEAP_START;

static vmm_range_t s_free_ranges[VMM_MAX_FREE_RANGES];
static size_t s_free_range_count;

static uint64_t *get_next_level(uint64_t *table, size_t index, bool allocate) {
  // a really stupid walker
  // should work tho
  if (table[index] & VMM_PRESENT) {

    if (table[index] & VMM_HUGE) {
      kassert(allocate && "Unexpected 2mb huge page when walking");

      uintptr_t huge_phys = table[index] & ADDR_MASK;
      uint64_t huge_flags = table[index] & ~ADDR_MASK & ~VMM_HUGE;

      uintptr_t new_pt_frame = pmm_alloc();
      kassert(new_pt_frame != 0 &&
              "Out of physical memory when splitting huge page");
      uint64_t *new_pt = (uint64_t *)p2v(new_pt_frame);

      for (int i = 0; i < 512; i++)
        new_pt[i] = (huge_phys + i * PAGE_SIZE) | huge_flags | VMM_PRESENT;

      table[index] = new_pt_frame | VMM_PRESENT | VMM_WRITABLE | VMM_USER;
      return new_pt;
    }
    return (uint64_t *)p2v(table[index] & ADDR_MASK);
  }

  if (!allocate)
    return NULL;

  uintptr_t frame = pmm_alloc();
  kassert(frame != 0 && "Out of physical memory for page table!");
  uint64_t *newtable = (uint64_t *)p2v(frame);
  for (int i = 0; i < 512; i++)
    newtable[i] = 0;

  table[index] = frame | VMM_PRESENT | VMM_WRITABLE | VMM_USER;
  return newtable;
}

static inline void flush_if_active(vmm_pagemap_t *pm, uintptr_t virt) {
  uint64_t cr3;
  __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
  if (cr3 == pm->pml4_phys)
    __asm__ volatile("invlpg (%0)" ::"r"(virt) : "memory");
}

void vmm_map_page(vmm_pagemap_t *pm, uintptr_t virt, uintptr_t phys,
                  uint64_t flags) {
  kassert((virt % PAGE_SIZE) == 0 && "Unaligned virtual address");
  kassert((phys % PAGE_SIZE) == 0 && "Unaligned physical address");

  uint64_t *pml4 = (uint64_t *)p2v(pm->pml4_phys);
  uint64_t *pdpt = get_next_level(pml4, PML4_INDEX(virt), true);
  uint64_t *pd = get_next_level(pdpt, PDPT_INDEX(virt), true);
  uint64_t *pt = get_next_level(pd, PD_INDEX(virt), true);

  pt[PT_INDEX(virt)] = (phys & ADDR_MASK) | flags | VMM_PRESENT;
  flush_if_active(pm, virt);
}

void vmm_unmap_page(vmm_pagemap_t *pm, uintptr_t virt) {
  uint64_t *pml4 = (uint64_t *)p2v(pm->pml4_phys);
  uint64_t *pdpt = get_next_level(pml4, PML4_INDEX(virt), false);
  uint64_t *pd = pdpt ? get_next_level(pdpt, PDPT_INDEX(virt), false) : NULL;
  uint64_t *pt = pd ? get_next_level(pd, PD_INDEX(virt), false) : NULL;
  if (!pt)
    return;

  pt[PT_INDEX(virt)] = 0;
  flush_if_active(pm, virt);
}

void vmm_map_range(vmm_pagemap_t *pm, uintptr_t virt, uintptr_t phys,
                   size_t length, uint64_t flags) {
  size_t pages = div_ceil(length, PAGE_SIZE);
  for (size_t i = 0; i < pages; i++)
    vmm_map_page(pm, virt + i * PAGE_SIZE, phys + i * PAGE_SIZE, flags);
}

void vmm_unmap_range(vmm_pagemap_t *pm, uintptr_t virt, size_t length) {
  size_t pages = div_ceil(length, PAGE_SIZE);
  for (size_t i = 0; i < pages; i++)
    vmm_unmap_page(pm, virt + i * PAGE_SIZE);
}

void vmm_switch_pagemap(vmm_pagemap_t *pm) {
  __asm__ volatile("mov %0, %%cr3" ::"r"(pm->pml4_phys) : "memory");
}

void vmm_map_page_2mb(vmm_pagemap_t *pm, uintptr_t virt, uintptr_t phys,
                      uint64_t flags) {
  kassert((virt % 0x200000) == 0 &&
          "unaligned 2mb virtual address in vmm_map_page_2mb");
  kassert((phys % 0x200000) == 0 &&
          "unaligned 2mb physical address in vmm_map_page_2mb");

  uint64_t *pml4 = (uint64_t *)p2v(pm->pml4_phys);
  uint64_t *pdpt = get_next_level(pml4, PML4_INDEX(virt), true);
  uint64_t *pd = get_next_level(pdpt, PDPT_INDEX(virt), true);

  pd[PD_INDEX(virt)] = (phys & ADDR_MASK) | flags | VMM_HUGE | VMM_PRESENT;
  flush_if_active(pm, virt);
}

void vmm_init(void) {
  klog_info("Initializing VMM...");

  uintptr_t pml4_phys = pmm_alloc();
  kassert(pml4_phys != 0 && "Failed to allocate PML4");
  uint64_t *pml4 = (uint64_t *)p2v(pml4_phys);
  for (int i = 0; i < 512; i++)
    pml4[i] = 0;

  g_kernel_pagemap.pml4_phys = pml4_phys;

  uintptr_t highest_phys = 0;
  for (size_t i = 0; i < memmap_request.response->entry_count; i++) {
    struct limine_memmap_entry *entry = memmap_request.response->entries[i];
    uintptr_t end = entry->base + entry->length;
    if (end > highest_phys)
      highest_phys = end;
  }

  uintptr_t hhdm_offset = hhdm_request.response->offset;
  uintptr_t mapped_end = div_ceil(highest_phys, 0x200000) * 0x200000;
  for (uintptr_t phys = 0; phys < mapped_end; phys += 0x200000) {
    vmm_map_page_2mb(&g_kernel_pagemap, phys + hhdm_offset, phys,
                     VMM_WRITABLE | VMM_NX);
  }

  extern char __kernel_end[];
  uintptr_t kern_phys = kernel_address_request.response->physical_base;
  uintptr_t kern_virt = kernel_address_request.response->virtual_base;
  size_t kernel_size = (uintptr_t)__kernel_end - kern_virt;
  size_t kernel_pages = div_ceil(kernel_size, PAGE_SIZE);
  for (size_t i = 0; i < kernel_pages; i++) {
    vmm_map_page(&g_kernel_pagemap, kern_virt + i * PAGE_SIZE,
                 kern_phys + i * PAGE_SIZE, VMM_WRITABLE);
  }

  struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
  uintptr_t fb_phys = v2p(fb->address);
  size_t fb_size = fb->pitch * fb->height;
  size_t fb_pages = div_ceil(fb_size, PAGE_SIZE);
  for (size_t i = 0; i < fb_pages; i++) {
    vmm_map_page(&g_kernel_pagemap, fb_phys + hhdm_offset + i * PAGE_SIZE,
                 fb_phys + i * PAGE_SIZE, VMM_WRITABLE | VMM_NOCACHE | VMM_NX);
  }

  vmm_switch_pagemap(&g_kernel_pagemap);
  klog_ok("VMM Initialized, switched to kernel pagemap");
}

void vmm_page_fault_handler(uint64_t error_code, uintptr_t faulting_addr) {
  bool present = error_code & (1 << 0);
  bool write = error_code & (1 << 1);
  bool user = error_code & (1 << 2);
  bool reserved = error_code & (1 << 3);
  bool fetch = error_code & (1 << 4);

  kpanic("Page fault at 0x%llx (%s, %s, %s%s%s)", faulting_addr,
         present ? "present" : "not-present", write ? "write" : "read",
         user ? "user" : "kernel", reserved ? ", reserved bit set" : "",
         fetch ? ", instruction fetch" : "");
}

static void *vmm_map_new_pages(uintptr_t base, size_t pages, uint64_t flags) {
  for (size_t i = 0; i < pages; i++) {
    uintptr_t phys = pmm_alloc();
    kassert(phys != 0 && "Out of physical memory");
    vmm_map_page(&g_kernel_pagemap, base + i * PAGE_SIZE, phys, flags);
  }
  return (void *)base;
}

void *vmm_alloc(size_t size, uint64_t flags) {
  size_t pages = div_ceil(size, PAGE_SIZE);
  uintptr_t base = 0;

  for (size_t i = 0; i < s_free_range_count; i++) {
    if (s_free_ranges[i].used && s_free_ranges[i].pages >= pages) {
      base = s_free_ranges[i].base;
      if (s_free_ranges[i].pages > pages) {
        s_free_ranges[i].base += pages * PAGE_SIZE;
        s_free_ranges[i].pages -= pages;
      } else {
        s_free_ranges[i].used = false;
      }
      break;
    }
  }

  if (base == 0) {
    base = s_heap_bump;
    kassert(base + pages * PAGE_SIZE <= VMM_HEAP_END &&
            "Kernel heap exhausted");
    s_heap_bump += pages * PAGE_SIZE;
  }

  return vmm_map_new_pages(base, pages, flags);
}

void vmm_free(void *ptr, size_t size) {
  uintptr_t base = (uintptr_t)ptr;
  size_t pages = div_ceil(size, PAGE_SIZE);

  for (size_t i = 0; i < pages; i++) {
    // TODO: abstractize allat
    uintptr_t virt = base + i * PAGE_SIZE;
    uint64_t *pml4 = (uint64_t *)p2v(g_kernel_pagemap.pml4_phys);
    uint64_t *pdpt = get_next_level(pml4, PML4_INDEX(virt), false);
    uint64_t *pd = pdpt ? get_next_level(pdpt, PDPT_INDEX(virt), false) : NULL;
    uint64_t *pt = pd ? get_next_level(pd, PD_INDEX(virt), false) : NULL;
    if (pt && (pt[PT_INDEX(virt)] & VMM_PRESENT))
      pmm_free(pt[PT_INDEX(virt)] & ADDR_MASK);
    vmm_unmap_page(&g_kernel_pagemap, virt);
  }

  kassert(s_free_range_count < VMM_MAX_FREE_RANGES &&
          "Out of free-range tracking slots");
  s_free_ranges[s_free_range_count++] = (vmm_range_t){base, pages, true};
}
