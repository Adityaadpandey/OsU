/*
 * tss.c - Task State Segment Implementation
 */

#include "tss.h"
#include "string.h"

/* TSS entry */
static tss_t tss;

/* Kernel stack for syscalls */
static uint8_t kernel_stack[4096] __attribute__((aligned(16)));

/* External function to load TSS (defined in assembly) */
extern void tss_flush(void);

/* GDT TSS descriptor (will be set up in idt.c) */
extern void gdt_set_tss(uint32_t base, uint32_t limit);

void tss_init(void) {
    /* Clear TSS */
    memset(&tss, 0, sizeof(tss));

    /* Set up kernel stack */
    tss.ss0 = 0x10;  /* Kernel data segment */
    tss.esp0 = (uint32_t)&kernel_stack[sizeof(kernel_stack)];

    /* Set I/O map base to beyond TSS limit (no I/O allowed) */
    tss.iomap_base = sizeof(tss);

    /* Set up TSS descriptor in GDT */
    gdt_set_tss((uint32_t)&tss, sizeof(tss) - 1);

    /* Load TSS */
    tss_flush();
}

void tss_set_kernel_stack(uint32_t stack) {
    tss.esp0 = stack;
}
