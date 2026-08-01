#include "limine.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <allocator/pmm.h>
#include <fb/fbtext.h>

#include <io/idt.h>
#include <limine/requests.h>
#include <panic.h>
#include <utils/archname.h>
#include <utils/kassert.h>
#include <utils/memory.h>
#include <utils/printk.h>

const char *mchunk_type_to_str(uint32_t t) {
  switch (t) {
  case LIMINE_MEMMAP_USABLE:
    return "Usable";
  case LIMINE_MEMMAP_RESERVED:
    return "Reserved";
  case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
    return "Reclaimable bootloader memory";
  case LIMINE_MEMMAP_EXECUTABLE_AND_MODULES:
    return "Kernel memory";
  case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
    return "Reclaimable ACPI tables";
  case LIMINE_MEMMAP_BAD_MEMORY:
    return "Bad memory";
  case LIMINE_MEMMAP_ACPI_NVS:
    return "ACPI NVS";
  case LIMINE_MEMMAP_FRAMEBUFFER:
    return "Framebuffer";
  case LIMINE_MEMMAP_RESERVED_MAPPED:
    return "Reserved mapped";
  default:
    return "(Unusable)";
  }
}

void kmain(void) {
  kassert(LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) != false);
  kassert(framebuffer_request.response != NULL &&
          framebuffer_request.response->framebuffer_count >= 1);
  kassert(memmap_request.response != NULL &&
          memmap_request.response->entry_count >= 1);
  kassert(hhdm_request.response != NULL);

  struct limine_framebuffer *framebuffer =
      framebuffer_request.response->framebuffers[0];

  fb_set_limine_framebuffer(framebuffer);
  fb_clear();

  klog_info("Starting kernel with %d-bit on %s, welcome!\n", sizeof(void *) * 8,
            arch_name());

  idt_init();
  pmm_init();

  //  idt_init();

  uintptr_t at = pmm_alloc();
  pmm_free(at);
  pmm_free(at);

  // int b = 1 / 0;
  // klog_info("%d", b);

  halt_catchfire();
}
