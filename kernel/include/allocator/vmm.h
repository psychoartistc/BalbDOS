#ifndef VMM_H_
#define VMM_H_

#include <allocator/pmm.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// goal is to simulate continuous memory and load our own page tables
// we will need the page table itself and an entry to it,
// an entry should and will store flags and the address
// then walk

// page table entry flags
#define VMM_PRESENT (1ULL << 0)
#define VMM_WRITABLE (1ULL << 1)
#define VMM_USER (1ULL << 2)
#define VMM_WRITETHRU (1ULL << 3)
#define VMM_NOCACHE (1ULL << 4)
#define VMM_HUGE (1ULL << 7)
#define VMM_GLOBAL (1ULL << 8)
#define VMM_NX (1ULL << 63)

#define PML4_INDEX(a) (((a) >> 39) & 0x1FF)
#define PDPT_INDEX(a) (((a) >> 30) & 0x1FF)
#define PD_INDEX(a) (((a) >> 21) & 0x1FF)
#define PT_INDEX(a) (((a) >> 12) & 0x1FF)
#define ADDR_MASK 0x000FFFFFFFFFF000ULL

// 4 gb for our kernel heap
#define VMM_HEAP_START 0xffff900000000000ULL
#define VMM_HEAP_END 0xffff9000ffffffffULL

#define VMM_MAX_FREE_RANGES 256

typedef struct {
  uintptr_t base;
  size_t pages;
  bool used;
} vmm_range_t;

// this vmm should use 2mb pages for HHDM
// and 4kb pages for everything else,
// there is also some simple rangeing
typedef struct {
  uintptr_t pml4_phys;
} vmm_pagemap_t;

extern vmm_pagemap_t g_kernel_pagemap;

void vmm_init(void);

void vmm_map_page(vmm_pagemap_t *pm, uintptr_t virt, uintptr_t phys,
                  uint64_t flags);
void vmm_unmap_page(vmm_pagemap_t *pm, uintptr_t virt);
void vmm_map_range(vmm_pagemap_t *pm, uintptr_t virt, uintptr_t phys,
                   size_t length, uint64_t flags);
void vmm_unmap_range(vmm_pagemap_t *pm, uintptr_t virt, size_t length);

void vmm_switch_pagemap(vmm_pagemap_t *pm);

void *vmm_alloc(size_t size, uint64_t flags);
void vmm_free(void *ptr, size_t size);

void vmm_page_fault_handler(uint64_t error_code, uintptr_t faulting_addr);

#endif
