/*
 * v86.c - Virtual 8086 Mode Implementation
 * Allows calling BIOS interrupts from protected mode
 */

#include "v86.h"
#include "string.h"
#include "vga.h"

/* V86 mode state */
static volatile int v86_active = 0;
static bios_regs_t *v86_regs = 0;
static volatile int v86_done = 0;

/* Low memory area for V86 code (below 1MB) */
#define V86_STACK_SEG    0x8000
#define V86_STACK_OFF    0xFFFE
#define V86_CODE_SEG     0x7000
#define V86_CODE_OFF     0x0000

/* Real mode interrupt vector table at 0x0000 */
#define IVT_BASE 0x00000000

/* V86 task state segment location */
#define V86_TSS_ESP0     0x9FC00

/* Copy of original interrupt handlers */
static uint32_t saved_ivt[256 * 2];

void v86_init(void) {
    /* Save IVT - we may need to restore it later */
    memcpy(saved_ivt, (void *)IVT_BASE, sizeof(saved_ivt));
    v86_active = 0;
    v86_done = 0;
}

int v86_is_active(void) {
    return v86_active;
}

/* Emulate a real-mode instruction in V86 mode
 * This is called from the GP fault handler
 */
static int emulate_instruction(uint8_t *ip, uint32_t *regs) {
    uint8_t opcode = *ip;

    /* Get segment:offset from stack frame */
    uint16_t *v86_sp = (uint16_t *)(((regs[11] << 4) + regs[3]) & 0xFFFFF); /* SS:SP */

    switch (opcode) {
        case 0xCD: { /* INT n */
            uint8_t int_num = *(ip + 1);

            /* If this is our exit interrupt (INT 0xFF), we're done */
            if (int_num == 0xFF) {
                v86_done = 1;
                return 2;  /* Skip 2 bytes */
            }

            /* Get interrupt vector from IVT */
            uint16_t *ivt = (uint16_t *)IVT_BASE;
            uint16_t new_ip = ivt[int_num * 2];
            uint16_t new_cs = ivt[int_num * 2 + 1];

            /* Push flags, CS, IP onto V86 stack */
            v86_sp--;
            *v86_sp = (uint16_t)regs[10];  /* FLAGS */
            v86_sp--;
            *v86_sp = (uint16_t)regs[9];   /* CS */
            v86_sp--;
            *v86_sp = (uint16_t)(regs[8] + 2);  /* IP (after INT instruction) */

            /* Update SP in stack frame */
            regs[3] = (uint32_t)((uintptr_t)v86_sp & 0xFFFF);

            /* Update CS:IP to interrupt handler */
            regs[8] = new_ip;
            regs[9] = new_cs;

            return 0;  /* Don't advance IP, we set it */
        }

        case 0xCF: { /* IRET */
            /* Pop IP, CS, FLAGS from V86 stack */
            uint16_t new_ip = *v86_sp++;
            uint16_t new_cs = *v86_sp++;
            uint16_t new_flags = *v86_sp++;

            /* Update SP */
            regs[3] = (uint32_t)((uintptr_t)v86_sp & 0xFFFF);

            /* Update CS:IP:FLAGS */
            regs[8] = new_ip;
            regs[9] = new_cs;
            regs[10] = (regs[10] & 0xFFFF0000) | new_flags;

            return 0;
        }

        case 0xFA: /* CLI */
            regs[10] &= ~EFLAGS_IF;
            return 1;

        case 0xFB: /* STI */
            regs[10] |= EFLAGS_IF;
            return 1;

        case 0x9C: { /* PUSHF */
            v86_sp--;
            *v86_sp = (uint16_t)regs[10];
            regs[3] = (uint32_t)((uintptr_t)v86_sp & 0xFFFF);
            return 1;
        }

        case 0x9D: { /* POPF */
            uint16_t flags = *v86_sp++;
            regs[3] = (uint32_t)((uintptr_t)v86_sp & 0xFFFF);
            regs[10] = (regs[10] & 0xFFFF0000) | flags;
            return 1;
        }

        case 0xF4: /* HLT */
            return 1;

        case 0xEE: /* OUT DX, AL */
        case 0xEF: /* OUT DX, AX */
        case 0xEC: /* IN AL, DX */
        case 0xED: /* IN AX, DX */
            /* Just skip I/O instructions for now */
            return 1;

        default:
            /* Unknown instruction - can't emulate */
            return -1;
    }
}

/* Handle GP fault from V86 mode */
int v86_handle_gpf(uint32_t *esp) {
    if (!v86_active) {
        return 0;
    }

    /* Stack frame layout for V86 GP fault:
     * ESP+0:  EIP
     * ESP+4:  CS
     * ESP+8:  EFLAGS
     * ESP+12: ESP
     * ESP+16: SS
     * ESP+20: ES
     * ESP+24: DS
     * ESP+28: FS
     * ESP+32: GS
     */

    uint32_t eflags = esp[2];
    if (!(eflags & EFLAGS_VM)) {
        return 0;  /* Not V86 mode */
    }

    /* Get instruction pointer */
    uint16_t cs = esp[1] & 0xFFFF;
    uint16_t ip = esp[0] & 0xFFFF;
    uint8_t *instr = (uint8_t *)((cs << 4) + ip);

    /* Pass full register context */
    int advance = emulate_instruction(instr, esp);

    if (advance < 0) {
        /* Can't emulate - exit V86 mode with error */
        v86_done = 1;
        return 1;
    }

    if (advance > 0) {
        esp[0] += advance;  /* Advance IP */
    }

    if (v86_done) {
        /* Copy registers back to caller */
        if (v86_regs) {
            v86_regs->ax = esp[7] & 0xFFFF;   /* Rough approximation */
            v86_regs->bx = esp[6] & 0xFFFF;
            v86_regs->cx = esp[5] & 0xFFFF;
            v86_regs->dx = esp[4] & 0xFFFF;
        }

        v86_active = 0;
        /* Return to protected mode - this needs special handling */
    }

    return 1;
}

/* V86 frame structure for entering V86 mode */
typedef struct {
    uint32_t gs, fs, ds, es;    /* Segment registers */
    uint32_t ss, esp;           /* V86 stack */
    uint32_t eflags;            /* With VM bit set */
    uint32_t cs, eip;           /* V86 code */
    uint32_t eax, ebx, ecx, edx; /* General purpose */
} v86_frame_t;

static v86_frame_t v86_frame;

/* Simple V86 BIOS call - sets up V86 mode and calls interrupt
 * Note: This is a simplified implementation
 */
int v86_bios_call(uint8_t int_num, bios_regs_t *regs) {
    /* For now, use a simpler approach:
     * Build a small real-mode code snippet that does:
     *   - Set up registers
     *   - INT <int_num>
     *   - INT 0xFF (our exit signal)
     * Then enter V86 mode to execute it
     */

    /* Put code at V86_CODE_SEG:V86_CODE_OFF */
    uint8_t *code = (uint8_t *)((V86_CODE_SEG << 4) + V86_CODE_OFF);

    /* Generate: INT int_num; INT 0xFF (exit) */
    code[0] = 0xCD;         /* INT */
    code[1] = int_num;
    code[2] = 0xCD;         /* INT */
    code[3] = 0xFF;         /* Exit V86 */
    code[4] = 0xF4;         /* HLT (just in case) */

    v86_regs = regs;
    v86_active = 1;
    v86_done = 0;

    /* Set up V86 frame */
    v86_frame.gs = 0;
    v86_frame.fs = 0;
    v86_frame.ds = 0;
    v86_frame.es = 0;
    v86_frame.ss = V86_STACK_SEG;
    v86_frame.esp = V86_STACK_OFF;
    v86_frame.eflags = EFLAGS_VM | EFLAGS_IF | 0x3000; /* VM=1, IF=1, IOPL=3 */
    v86_frame.cs = V86_CODE_SEG;
    v86_frame.eip = V86_CODE_OFF;
    v86_frame.eax = regs->ax;
    v86_frame.ebx = regs->bx;
    v86_frame.ecx = regs->cx;
    v86_frame.edx = regs->dx;

    /* Enter V86 mode */
    __asm__ volatile (
        /* Push V86 stack frame */
        "pushl %0\n\t"         /* GS */
        "pushl %1\n\t"         /* FS */
        "pushl %2\n\t"         /* DS */
        "pushl %3\n\t"         /* ES */
        "pushl %4\n\t"         /* SS */
        "pushl %5\n\t"         /* ESP */
        "pushl %6\n\t"         /* EFLAGS */
        "pushl %7\n\t"         /* CS */
        "pushl %8\n\t"         /* EIP */
        /* Set up registers for BIOS call */
        "movl %9, %%eax\n\t"
        "movl %10, %%ebx\n\t"
        "movl %11, %%ecx\n\t"
        "movl %12, %%edx\n\t"
        /* Enter V86 mode via IRET */
        "iret\n\t"
        :
        : "m"(v86_frame.gs), "m"(v86_frame.fs), "m"(v86_frame.ds), "m"(v86_frame.es),
          "m"(v86_frame.ss), "m"(v86_frame.esp), "m"(v86_frame.eflags),
          "m"(v86_frame.cs), "m"(v86_frame.eip),
          "m"(v86_frame.eax), "m"(v86_frame.ebx), "m"(v86_frame.ecx), "m"(v86_frame.edx)
        : "memory", "eax", "ebx", "ecx", "edx"
    );

    /* We shouldn't reach here - V86 mode handler will return us */

    return v86_done ? 0 : -1;
}
