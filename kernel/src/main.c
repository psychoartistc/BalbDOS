#include "limine.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <allocator/pmm.h>
#include <allocator/slab.h>
#include <allocator/vmm.h>
#include <drivers/api.h>
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

  klog_info("Starting the kernel with %d-bit on %s, welcome!",
            sizeof(void *) * 8, arch_name());
  printk("* Disable debug outputs by undefining ENABLE_DEBUG\n");
  printk("* Play around with the allocator or framebuffer in kmain()\n");
  printk("* Write cool drivers with the driver API\n");
  printk("* Shell support will be here when a proper userland is made because "
         "kernel shells suck\n");
  printk("* Want it faster? Feel free to contribute at "
         "https://github.com/psychoartistc/lsd\n\n");

  // these initialization routines
  // are crucial, so they panic on failure
  idt_init();
  pmm_init();
  vmm_init();
  allocator_init();

  char buffer[FORMATSIZE_BUFSIZE];
  formatsize(buffer, pmm_get_pages_count() * PAGE_SIZE);
  klog_info("Total of %s physical memory available", buffer);
  // int b = 1 / 0;
  // klog_info("%d", b);'

  output_drivers();
  init_drivers();

cleanup:
  cleanup_drivers();
  halt_catchfire();
}
