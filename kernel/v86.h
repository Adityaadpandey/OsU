/*
 * v86.h - Virtual 8086 Mode for BIOS calls
 * Allows calling real-mode BIOS interrupts from protected mode
 */

#ifndef V86_H
#define V86_H

#include <stdint.h>

/* V86 EFLAGS bit */
#define EFLAGS_VM     0x00020000  /* Virtual 8086 mode */
#define EFLAGS_IF     0x00000200  /* Interrupt flag */
#define EFLAGS_IOPL   0x00003000  /* I/O Privilege Level */

/* Registers for BIOS call */
typedef struct {
    uint16_t ax, bx, cx, dx;
    uint16_t si, di, bp;
    uint16_t ds, es, fs, gs;
    uint16_t flags;
} bios_regs_t;

/* Initialize V86 mode support */
void v86_init(void);

/* Call a BIOS interrupt using V86 mode
 * int_num: interrupt number (e.g., 0x10 for video)
 * regs: input/output registers
 * Returns 0 on success, -1 on failure
 */
int v86_bios_call(uint8_t int_num, bios_regs_t *regs);

/* Check if currently in V86 mode */
int v86_is_active(void);

/* Handle GP fault from V86 mode
 * Returns 1 if handled, 0 if not a V86 fault
 */
int v86_handle_gpf(uint32_t *esp);

#endif /* V86_H */
