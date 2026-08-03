#ifndef IDT_H_
#define IDT_H_

#include <stdint.h>

#include <utils/printk.h>

struct idt_entry {
  uint16_t isr_low;
  uint16_t kernel_cs;
  uint8_t ist;
  uint8_t attributes;
  uint16_t isr_mid;
  uint32_t isr_high;
  uint32_t reserved;
} __attribute__((packed));

struct idt_ptr {
  uint16_t limit;
  uint64_t base;
} __attribute__((packed));

typedef struct {

  uint64_t rax, rbx, rcx, rdx, rsi, rdi, r8, r9, r10, r11, r12, r13, r14, r15,
      rbp;
  uint64_t int_no, error_code;

  uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed)) cpu_registers_x86_t;

typedef struct {
  uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
  uint64_t rax, rbx, rcx, rdx, rsi, rdi, rbp;
  uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed)) interrupt_frame_t;

extern struct idt_entry g_idt[256];
extern struct idt_ptr g_idtr;
extern uint64_t g_exception_handlers_table[32];

extern const char *g_exception_messages[];

extern void page_fault_stub(void);

void page_fault_handler(void);
void c_exception_handler(cpu_registers_x86_t *regs);
void idt_set_descriptor(uint8_t vector, void *isr, uint8_t attributes);
void idt_init(void);

#endif
