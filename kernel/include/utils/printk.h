#ifndef PRINTK_H_
#define PRINTK_H_

#include <fb/fbtext.h>
#include <utils/printf.h>

#define ENABLE_DEBUG

int printk(const char *fmt, ...);

int klog_ok(const char *fmt, ...);
int klog_info(const char *fmt, ...);
int klog_warn(const char *fmt, ...);
int klog_error(const char *fmt, ...);
int klog_custom(uint32_t color, const char *prefix, const char *fmt, ...);
int klog_debug(const char *fmt, ...);

#endif
