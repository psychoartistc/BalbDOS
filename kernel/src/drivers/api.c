#include <drivers/api.h>
#include <utils/printk.h>

void init_drivers() {
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
  }
}

void cleanup_drivers() {
  for (struct driver *d = __drivers_start; d < __drivers_end; d++) {
    if (!d->exit)
      continue;

    d->exit();
  }
}
