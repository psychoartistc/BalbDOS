#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <allocator/pmm.h>
#include <allocator/slab.h>
#include <allocator/vmm.h>
#include <drivers/api.h>
#include <fb/fbtext.h>

#include <acpi/table.h>
#include <io/idt.h>
#include <limine/requests.h>
#include <panic.h>

#include <utils/kassert.h>
#include <utils/memory.h>
#include <utils/printk.h>

void kmain(void) {
  request_asserts();

  fb_init();

  klog_info("Starting the kernel with %d-bit on x86-64, welcome!",
            sizeof(void *) * 8);
  printk("* Disable debug outputs by undefining ENABLE_DEBUG\n");
  printk("* Play around with the allocator or framebuffer in kmain()\n");
  printk("* Write cool drivers with the driver API\n");
  printk("* Enable blue terminal theme by enabling the FB_BLUE_THEME macro\n");
  printk("* Shell support will be here when a proper userland is made because "
         "kernel shells suck\n");
  printk("* Want it faster? Feel free to contribute at "
         "https://github.com/psychoartistc/BalbDOS\n\n");

  // these initialization routines
  // are crucial, so they panic on failure
  idt_init();
  pmm_init();
  vmm_init();
  allocator_init();
  acpi_init();

  // int b = 1 / 0;
  // klog_info("%d", b);'

  init_drivers();
  klog_ok("Drivers initialized");

cleanup:
  cleanup_drivers();

  // kpanic("Kernel panic test");
  halt_catchfire();
}
