#include <io/idt.h>
#include <panic.h>
#include <stdint.h>

#include <allocator/vmm.h>

const char *g_exception_messages[] = {"Divide Error",
                                      "Debug",
                                      "NMI Interrupt",
                                      "Breakpoint",
                                      "Overflow",
                                      "BOUND Range Exceeded",
                                      "Invalid Opcode",
                                      "Device Not Available",
                                      "Double Fault",
                                      "Coprocessor Segment Overrun",
                                      "Invalid TSS",
                                      "Segment Not Present",
                                      "Stack-Segment Fault",
                                      "General Protection",
                                      "Page Fault",
                                      "Reserved",
                                      "x87 FPU Floating-Point Error",
                                      "Alignment Check",
                                      "Machine Check",
                                      "SIMD Floating-Point Exception",
                                      "Virtualization Exception",
                                      "Control Protection Exception",
                                      "Reserved",
                                      "Reserved",
                                      "Reserved",
                                      "Reserved",
                                      "Reserved",
                                      "Reserved",
                                      "Hypervisor Injection Exception",
                                      "VMM Communication Exception",
                                      "Security Exception",
                                      "Reserved"};

struct idt_entry g_idt[256] = {0};
struct idt_ptr g_idtr = {0};

static inline uint16_t get_cs(void) {
  uint16_t cs;
  __asm__ volatile("mov %%cs, %0" : "=r"(cs));
  return cs;
}

void idt_set_descriptor(uint8_t vector, void *isr, uint8_t attributes) {
  uint64_t addr = (uint64_t)isr;

  g_idt[vector].isr_low = addr & 0xFFFF;
  g_idt[vector].kernel_cs = get_cs();
  g_idt[vector].ist = 0;
  g_idt[vector].attributes = attributes;
  g_idt[vector].isr_mid = (addr >> 16) & 0xFFFF;
  g_idt[vector].isr_high = (addr >> 32) & 0xFFFFFFFF;
  g_idt[vector].reserved = 0;
}

void c_exception_handler(cpu_registers_t *regs) {
  if (regs->int_no == 14) {
    uint64_t cr2;
    __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
    vmm_page_fault_handler(regs->error_code, (uintptr_t)cr2);
    return;
  }
  if (regs->int_no < 32) {
    const char *name = g_exception_messages[regs->int_no];
    kpanic("CPU exception: %s (at RIP 0x%p)", name, regs->rip);
  }
}

void idt_init(void) {
  g_idtr.limit = (sizeof(struct idt_entry) * 256) - 1;
  g_idtr.base = (uint64_t)&g_idt;

  for (int i = 0; i < 32; i++) {
    idt_set_descriptor(i, (void *)g_exception_handlers_table[i], 0x8E);
  }

  __asm__ volatile("lidt %0" : : "m"(g_idtr));
  __asm__ volatile("sti");

  klog_ok("IDT Initialized");
}
