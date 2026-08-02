#include <drivers/api.h>
#include <utils/printk.h>

DRIVER_NAME("Test driver");
DRIVER_DESCRIPTION("Description");
DRIVER_VERSION("1.0.0");
DRIVER_LICENSE(DRVLICENSE_GPL3);

static int test_init() {
  // printk("Hello world!\n");
  return 0;
}

static void test_exit() { /* printk("Goodbye world!\n");*/ }

driver_init(test_init);
driver_exit(test_exit);
