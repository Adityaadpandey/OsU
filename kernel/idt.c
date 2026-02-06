#include "idt.h"

#include "io.h"
#include "vga.h"

#define IDT_ENTRIES 256
#define KERNEL_CS 0x08
#define IDT_FLAG_INT_GATE 0x8E

typedef struct {
    uint16_t base_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t idt_ptr;
static interrupt_handler_t handlers[IDT_ENTRIES];

extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

static void idt_set_gate(uint8_t n, uint32_t base, uint16_t selector, uint8_t flags) {
    idt[n].base_low = (uint16_t)(base & 0xFFFF);
    idt[n].selector = selector;
    idt[n].zero = 0;
    idt[n].flags = flags;
    idt[n].base_high = (uint16_t)((base >> 16) & 0xFFFF);
}

static void pic_remap(void) {
    uint8_t a1 = inb(0x21);
    uint8_t a2 = inb(0xA1);

    outb(0x20, 0x11);
    io_wait();
    outb(0xA0, 0x11);
    io_wait();

    outb(0x21, 0x20);
    io_wait();
    outb(0xA1, 0x28);
    io_wait();

    outb(0x21, 0x04);
    io_wait();
    outb(0xA1, 0x02);
    io_wait();

    outb(0x21, 0x01);
    io_wait();
    outb(0xA1, 0x01);
    io_wait();

    outb(0x21, a1);
    outb(0xA1, a2);
}

void idt_register_handler(uint8_t n, interrupt_handler_t handler) {
    handlers[n] = handler;
}

void idt_init(void) {
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate((uint8_t)i, 0, 0, 0);
        handlers[i] = 0;
    }

    idt_set_gate(0, (uint32_t)isr0, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(1, (uint32_t)isr1, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(2, (uint32_t)isr2, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(3, (uint32_t)isr3, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(4, (uint32_t)isr4, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(5, (uint32_t)isr5, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(6, (uint32_t)isr6, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(7, (uint32_t)isr7, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(8, (uint32_t)isr8, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(9, (uint32_t)isr9, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(10, (uint32_t)isr10, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(11, (uint32_t)isr11, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(12, (uint32_t)isr12, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(13, (uint32_t)isr13, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(14, (uint32_t)isr14, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(15, (uint32_t)isr15, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(16, (uint32_t)isr16, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(17, (uint32_t)isr17, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(18, (uint32_t)isr18, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(19, (uint32_t)isr19, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(20, (uint32_t)isr20, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(21, (uint32_t)isr21, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(22, (uint32_t)isr22, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(23, (uint32_t)isr23, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(24, (uint32_t)isr24, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(25, (uint32_t)isr25, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(26, (uint32_t)isr26, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(27, (uint32_t)isr27, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(28, (uint32_t)isr28, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(29, (uint32_t)isr29, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(30, (uint32_t)isr30, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(31, (uint32_t)isr31, KERNEL_CS, IDT_FLAG_INT_GATE);

    pic_remap();

    idt_set_gate(32, (uint32_t)irq0, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(33, (uint32_t)irq1, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(34, (uint32_t)irq2, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(35, (uint32_t)irq3, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(36, (uint32_t)irq4, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(37, (uint32_t)irq5, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(38, (uint32_t)irq6, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(39, (uint32_t)irq7, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(40, (uint32_t)irq8, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(41, (uint32_t)irq9, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(42, (uint32_t)irq10, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(43, (uint32_t)irq11, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(44, (uint32_t)irq12, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(45, (uint32_t)irq13, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(46, (uint32_t)irq14, KERNEL_CS, IDT_FLAG_INT_GATE);
    idt_set_gate(47, (uint32_t)irq15, KERNEL_CS, IDT_FLAG_INT_GATE);

    outb(0x21, 0xFD);
    outb(0xA1, 0xFF);

    idt_ptr.limit = sizeof(idt_entry_t) * IDT_ENTRIES - 1;
    idt_ptr.base = (uint32_t)&idt;

    __asm__ volatile("lidt %0" : : "m"(idt_ptr));
}

void isr_handler_c(registers_t *r) {
    if (handlers[r->int_no]) {
        handlers[r->int_no](r);
    } else if (r->int_no < 32) {
        vga_printf("EXC %u err=%x\n", r->int_no, r->err_code);
    }

    if (r->int_no >= 32 && r->int_no <= 47) {
        if (r->int_no >= 40) {
            outb(0xA0, 0x20);
        }
        outb(0x20, 0x20);
    }
}
