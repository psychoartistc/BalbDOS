#include "fb/fbtext.h"
#include <utils/printk.h>

int printk(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  int l = vprintf_(fmt, args);
  va_end(args);

  return l;
}

int klog_ok(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  printk("[ ");
  fb_puts("OK", 0x008000, fb_get_background_color());
  printk(" ] ");
  int l = vprintf_(fmt, args);
  fb_putchar('\n', 0x000000, 0x000000);
  va_end(args);

  return l;
}
int klog_info(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  printk("[ ");
  fb_puts("INFO", 0x008000, fb_get_background_color());
  printk(" ] ");
  int l = vprintf_(fmt, args);
  fb_putchar('\n', 0x000000, 0x000000);
  va_end(args);

  return l;
}
int klog_warn(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  printk("[ ");
  fb_puts("WARN", 0xFFFF00, fb_get_background_color());
  printk(" ] ");
  int l = vprintf_(fmt, args);
  fb_putchar('\n', 0x000000, 0x000000);
  va_end(args);

  return l;
}
int klog_error(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  printk("[ ");
  fb_puts("ERROR", 0xFF0000, fb_get_background_color());
  printk(" ] ");
  int l = vprintf_(fmt, args);
  fb_putchar('\n', 0x000000, 0x000000);
  va_end(args);

  return l;
}

int klog_custom(uint32_t color, const char *prefix, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  printk("[ ");
  fb_puts(prefix, color, fb_get_background_color());
  printk(" ] ");
  int l = vprintf_(fmt, args);
  fb_putchar('\n', 0x000000, 0x000000);
  va_end(args);

  return l;
}

int klog_debug(const char *fmt, ...) {
#ifdef ENABLE_DEBUG
  va_list args;
  va_start(args, fmt);
  printk("[ ");
  fb_puts("DEBUG", 0x00FFFF, fb_get_background_color());
  printk(" ] ");
  int l = vprintf_(fmt, args);
  fb_putchar('\n', 0x000000, 0x000000);
  va_end(args);

  return l;
#endif
  return 0;
}
