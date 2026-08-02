#include <drivers/api.h>
#include <utils/printk.h>

void init_drivers() {
  link_driver_callbacks();
  for (struct driver *d = __drivers_start; d < __drivers_end; d++) {
    if (!d->init)
      continue;

    int result = d->init();
    if (result != 0)
      klog_error("Failed to initialize driver \"%s\", error code: %d", result);
  }
}

void output_drivers() {
  for (struct driver *d = __drivers_start; d < __drivers_end; d++) {
    printk("* Driver name: %s\n", d->name);
    printk("* Driver description: \'%s\'\n", d->description);
    printk("* Driver version: %s\n", d->version);
    printk("* Driver license: %s\n", d->license);
    printk("* Driver init handler: 0x%p\n", d->init);
    printk("* Driver exit handler: 0x%p\n", d->exit);
  }
}

void link_driver_callbacks() {
  for (struct driver *d = __drivers_start; d < __drivers_end; d++) {
    for (struct driver_exit_entry *e = __drivers_exit_start;
         e < __drivers_exit_end; e++) {
      // pointer comparison....
      // should work tho because for string literals
      // if they are same they have the same address
      if (e->name == d->name) {
        d->exit = e->exit;
        break;
      }
    }
  }
}

void cleanup_drivers() {
  for (struct driver *d = __drivers_start; d < __drivers_end; d++) {
    if (!d->exit)
      continue;

    d->exit();
  }
}
