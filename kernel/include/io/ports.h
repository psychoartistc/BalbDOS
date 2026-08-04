#ifndef IO_PORTS_H_
#define IO_PORTS_H_

#include <stdint.h>

static inline uint8_t inb(unsigned long port) {
  uint8_t value;
  __asm__ volatile("inb %w1, %b0" : "=a"(value) : "Nd"(port));
  return value;
}

static inline void outb(uint8_t value, unsigned long port) {
  __asm__ volatile("outb %b0, %w1" : : "a"(value), "Nd"(port));
}

#endif
