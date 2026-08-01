#include "limine.h"
#include "limine/limine.h"
#include <allocator/pmm.h>
#include <limine/requests.h>
#include <panic.h>
#include <stddef.h>
#include <utils/kassert.h>
#include <utils/printk.h>
static pmm_node_t s_pmmHead = {0};
static uint64_t s_pmmEntries = 0;

pmm_node_t *pmm_get_head() { return &s_pmmHead; }
static inline int is_memmap_entry_available(uint32_t t) {
  return t == LIMINE_MEMMAP_USABLE || t == LIMINE_MEMMAP_ACPI_RECLAIMABLE ||
         t == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE;
}

void pmm_map_acpi_pages() {
  struct limine_memmap_response *memmap = memmap_request.response;
  pmm_node_t *current = &s_pmmHead;
  for (uint64_t i = 0; i < memmap->entry_count; i++) {
    struct limine_memmap_entry *entry = memmap->entries[i];
    if (entry->type != LIMINE_MEMMAP_ACPI_RECLAIMABLE)
      continue;
    uintptr_t baseVirt = pmm_phys_to_virt(entry->base);
    size_t pages = entry->length / PAGE_SIZE;
    for (size_t j = 0; j < pages; j++) {
      pmm_node_t *node = (pmm_node_t *)baseVirt;
      current->next = node;
      current = current->next;
      s_pmmEntries++;
      baseVirt += PAGE_SIZE;
    }
  }
  current->next = NULL;
  klog_info("PMM: Mapped %llu pages (Reclaimable ACPI)", s_pmmEntries);
}

void pmm_map_bootloader_pages() {
  struct limine_memmap_response *memmap = memmap_request.response;
  pmm_node_t *current = &s_pmmHead;
  for (uint64_t i = 0; i < memmap->entry_count; i++) {
    struct limine_memmap_entry *entry = memmap->entries[i];
    if (entry->type != LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE)
      continue;
    uintptr_t baseVirt = pmm_phys_to_virt(entry->base);
    size_t pages = entry->length / PAGE_SIZE;
    for (size_t j = 0; j < pages; j++) {
      pmm_node_t *node = (pmm_node_t *)baseVirt;
      current->next = node;
      current = current->next;
      s_pmmEntries++;
      baseVirt += PAGE_SIZE;
    }
  }
  current->next = NULL;
  klog_info("PMM: Mapped %llu pages (Reclaimable bootloader)", s_pmmEntries);
}

void pmm_map_bootloader_pages();

void pmm_map_pages() {
  struct limine_memmap_response *memmap = memmap_request.response;
  pmm_node_t *current = &s_pmmHead;
  for (uint64_t i = 0; i < memmap->entry_count; i++) {
    struct limine_memmap_entry *entry = memmap->entries[i];
    if (entry->type != LIMINE_MEMMAP_USABLE)
      continue;
    uintptr_t baseVirt = pmm_phys_to_virt(entry->base);
    size_t pages = entry->length / PAGE_SIZE;
    for (size_t j = 0; j < pages; j++) {
      pmm_node_t *node = (pmm_node_t *)baseVirt;
      current->next = node;
      current = current->next;
      s_pmmEntries++;
      baseVirt += PAGE_SIZE;
    }
  }
  current->next = NULL;
  klog_info("PMM: Mapped %llu pages", s_pmmEntries);
}

uint64_t pmm_total_entries() { return s_pmmEntries; }
void pmm_init() {
  pmm_map_pages();
  klog_ok("PMM Initialization");
}
pmm_node_t *pmm_allocate_page() {
  pmm_node_t *node = s_pmmHead.next;
  if (node == NULL)
    return NULL;
  s_pmmHead.next = node->next;
  node->next = NULL;
  s_pmmEntries--;
  return node;
}
pmm_node_t *pmm_allocate_pages(size_t count) {
  kassert(count != 0);
  if (count == 1)
    return pmm_allocate_page();

  pmm_node_t *prev = &s_pmmHead;
  while (prev->next) {
    pmm_node_t *first = prev->next;
    pmm_node_t *last = first;
    size_t found = 1;
    while (found < count) {
      if (last->next == NULL)
        break;
      uintptr_t expected = (uintptr_t)last + PAGE_SIZE;
      if ((uintptr_t)last->next != expected)
        break;
      last = last->next;
      found++;
    }
    if (found == count) {
      prev->next = last->next;
      last->next = NULL;
      s_pmmEntries -= count;
      return first;
    }
    prev = first;
  }
  return NULL;
}

void pmm_free_page(pmm_node_t *node) {
  kassert(((uintptr_t)node & (PAGE_SIZE - 1)) == 0);
  pmm_node_t *prev = &s_pmmHead;
  while (prev->next && prev->next < node)
    prev = prev->next;
  kassert(prev->next != node);
  node->next = prev->next;
  prev->next = node;
  s_pmmEntries++;
}
void pmm_free_pages(pmm_node_t *node, size_t count) {
  if (node == NULL || count == 0)
    return;
  if (count == 1) {
    pmm_free_page(node);
    return;
  }
  pmm_node_t *last = node;
  for (size_t i = 1; i < count; i++) {
    pmm_node_t *next = (pmm_node_t *)((uintptr_t)last + PAGE_SIZE);
    last->next = next;
    last = next;
  }
  pmm_node_t *prev = &s_pmmHead;
  while (prev->next && prev->next < node)
    prev = prev->next;
  last->next = prev->next;
  prev->next = node;
  s_pmmEntries += count;
}
