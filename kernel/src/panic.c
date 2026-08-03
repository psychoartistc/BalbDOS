#include <fb/fbtext.h>
#include <panic.h>
#include <utils/archname.h>

void disable_interrupts(void) {
#if defined(__x86_64__)
  __asm__ __volatile__("cli" : : : "memory");
#elif defined(__aarch64__)
  __asm__ __volatile__("msr daifset, #2" : : : "memory");
#elif defined(__riscv)
  __asm__ __volatile__("csrci sstatus, 2" : : : "memory");
#elif defined(__loongarch64)
  __asm__ __volatile__("csrwr $zero, 0x0\n\t" : : : "memory");
#endif
}

void halt_catchfire(void) {
  for (;;) {
#if defined(__x86_64__)
    __asm__("hlt");
#elif defined(__aarch64__) || defined(__riscv)
    asm("wfi");
#elif defined(__loongarch64)
    asm("idle 0");
#endif
  }
}

void dump_registers() {

#if !defined(__x86_64__)
  printk("* Register dump not supported for current architecture");
#else
  struct panic_regs_x86 r;

  __asm__ volatile("mov %%rax,%0\n\t"
                   "mov %%rbx,%1\n\t"
                   "mov %%rcx,%2\n\t"
                   "mov %%rdx,%3\n\t"
                   "mov %%rsi,%4\n\t"
                   "mov %%rdi,%5\n\t"
                   "mov %%rbp,%6\n\t"
                   "mov %%rsp,%7\n\t"
                   "mov %%r8,%8\n\t"
                   "mov %%r9,%9\n\t"
                   "mov %%r10,%10\n\t"
                   "mov %%r11,%11\n\t"
                   "mov %%r12,%12\n\t"
                   "mov %%r13,%13\n\t"
                   "mov %%r14,%14\n\t"
                   "mov %%r15,%15\n\t"
                   "lea (%%rip), %%rax\n\t"
                   "mov %%rax,%16\n\t"
                   "pushfq\n\t"
                   "pop %17"
                   : "=m"(r.rax), "=m"(r.rbx), "=m"(r.rcx), "=m"(r.rdx),
                     "=m"(r.rsi), "=m"(r.rdi), "=m"(r.rbp), "=m"(r.rsp),
                     "=m"(r.r8), "=m"(r.r9), "=m"(r.r10), "=m"(r.r11),
                     "=m"(r.r12), "=m"(r.r13), "=m"(r.r14), "=m"(r.r15),
                     "=m"(r.rip), "=m"(r.rflags)
                   :
                   : "rax", "memory");

  printk("\n* Register dump for %s\n", arch_name());

  printk("* RAX: %p  RBX: %p\n", r.rax, r.rbx);
  printk("* RCX: %p  RDX: %p\n", r.rcx, r.rdx);
  printk("* RSI: %p  RDI: %p\n", r.rsi, r.rdi);
  printk("* RBP: %p  RSP: %p\n", r.rbp, r.rsp);
  printk("* R8:  %p  R9:  %p\n", r.r8, r.r9);
  printk("* R10: %p  R11: %p\n", r.r10, r.r11);
  printk("* R12: %p  R13: %p\n", r.r12, r.r13);
  printk("* R14: %p  R15: %p\n", r.r14, r.r15);
  printk("* RIP: %p  RFL: %p\n", r.rip, r.rflags);
#endif
}

void kpanic(const char *fmt, ...) {
  disable_interrupts();

  printk("\n");
  fb_puts("!!! KERNEL PANIC !!!\n", 0xFF0000, 0x000000);
  printk("* The kernel has encountered an error and needs to quit. Sorry!\n");
  printk("* Error: ");
  va_list arg;
  va_start(arg, fmt);
  vprintf_(fmt, arg);
  va_end(arg);
  printk("\n");
  printk("* Don't worry; this is not your fault. Report this error to the "
         "developers\n");
  dump_registers();
  halt_catchfire();
}
