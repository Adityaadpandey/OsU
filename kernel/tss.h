/*
 * tss.h - Task State Segment
 * Required for ring transitions (user mode to kernel mode)
 */

#ifndef TSS_H
#define TSS_H

#include <stdint.h>

/* TSS structure for i386 */
typedef struct {
    uint32_t prev_tss;      /* Previous TSS (unused) */
    uint32_t esp0;          /* Stack pointer for ring 0 */
    uint32_t ss0;           /* Stack segment for ring 0 */
    uint32_t esp1;          /* Unused */
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax, ecx, edx, ebx;
    uint32_t esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed)) tss_t;

/* Initialize TSS and load it */
void tss_init(void);

/* Set kernel stack for ring transitions */
void tss_set_kernel_stack(uint32_t stack);

#endif /* TSS_H */
