/*
 * syscall.c - System Call Handler
 */

#include "syscall.h"
#include "idt.h"
#include "vga.h"
#include "keyboard.h"
#include "timer.h"
#include "process.h"

/* Current process PID (simple implementation) */
static int current_pid = 1;

/* Syscall handler called from assembly */
void syscall_handler(registers_t *regs) {
    uint32_t syscall_num = regs->eax;

    switch (syscall_num) {
        case SYS_EXIT:
            sys_exit((int)regs->ebx);
            break;

        case SYS_WRITE:
            regs->eax = (uint32_t)sys_write(
                (int)regs->ebx,
                (const char *)regs->ecx,
                (size_t)regs->edx
            );
            break;

        case SYS_READ:
            regs->eax = (uint32_t)sys_read(
                (int)regs->ebx,
                (char *)regs->ecx,
                (size_t)regs->edx
            );
            break;

        case SYS_GETPID:
            regs->eax = (uint32_t)sys_getpid();
            break;

        case SYS_SLEEP:
            sys_sleep(regs->ebx);
            break;

        case SYS_YIELD:
            sys_yield();
            break;

        default:
            regs->eax = (uint32_t)-1;  /* Unknown syscall */
            break;
    }
}

void syscall_init(void) {
    /* Register int 0x80 handler */
    idt_register_handler(0x80, syscall_handler);

    /* Make int 0x80 callable from ring 3 */
    extern void idt_set_gate_ring3(uint8_t n);
    idt_set_gate_ring3(0x80);
}

void sys_exit(int status) {
    vga_puts("[Process exited with status ");
    vga_print_dec((uint32_t)status);
    vga_puts("]\n");

    /* Return to kernel - in a real OS, would terminate process */
    /* For now, just halt this context */
    for (;;) {
        __asm__ volatile("hlt");
    }
}

int sys_write(int fd, const char *buf, size_t count) {
    if (fd == 1 || fd == 2) {  /* stdout or stderr */
        for (size_t i = 0; i < count; i++) {
            vga_putc(buf[i]);
        }
        return (int)count;
    }
    return -1;  /* Invalid fd */
}

int sys_read(int fd, char *buf, size_t count) {
    if (fd == 0) {  /* stdin */
        for (size_t i = 0; i < count; i++) {
            char c = keyboard_getchar();
            buf[i] = c;
            if (c == '\n') {
                return (int)(i + 1);
            }
        }
        return (int)count;
    }
    return -1;
}

int sys_getpid(void) {
    return current_pid;
}

void sys_sleep(uint32_t ms) {
    timer_sleep(ms);
}

void sys_yield(void) {
    scheduler_tick();
}
