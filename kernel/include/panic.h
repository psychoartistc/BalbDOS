#ifndef PANIC_H_
#define PANIC_H_

#include <utils/printk.h>

typedef struct panic_regs {
  uint64_t rax, rbx, rcx, rdx;
  uint64_t rsi, rdi;
  uint64_t rbp, rsp;
  uint64_t r8, r9, r10, r11;
  uint64_t r12, r13, r14, r15;
  uint64_t rip, rflags;
} panic_regs_t;

void dump_registers();

void halt_catchfire(void);
void disable_interrupts(void);

void kpanic(const char *fmt, ...);

#endif
