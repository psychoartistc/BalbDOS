#ifndef PMM_H_
#define PMM_H_

#include <limine/requests.h>
#include <utils/errors.h>
#include <utils/kassert.h>
#include <utils/printk.h>

#define PAGE_SIZE 4096

// physical to virtual address
static inline void *p2v(uintptr_t phys) {
  return (void *)(phys + hhdm_request.response->offset);
}

// virtual to physical address
static inline uintptr_t v2p(void *virt) {
  return (uintptr_t)virt - hhdm_request.response->offset;
}

// function type that should filter a region
typedef int (*pmm_filter_function)(struct limine_memmap_entry *);

typedef struct pmm_node {
  uintptr_t next;
} pmm_node_t;

uintptr_t pmm_get_freelist_head();
uintptr_t pmm_get_freelist_tail();

// fills the freelist
void pmm_init();

// returns a physical address to the free page,
// zero if out of physical memory
uintptr_t pmm_alloc();

// returns the allocated page back to the pmm's ownership
void pmm_free(uintptr_t at);

void pmm_reclaim_bootloader_pages();
void pmm_reclaim_acpi_pages();

#endif
