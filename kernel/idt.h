#ifndef IDT_H
#define IDT_H

#include <stdint.h>

typedef struct registers {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;
    uint32_t int_no;
    uint32_t err_code;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
} registers_t;

typedef void (*interrupt_handler_t)(registers_t *r);

void idt_init(void);
void idt_register_handler(uint8_t n, interrupt_handler_t handler);

#endif
