#include "idt.h"
#include "keyboard.h"
#include "memory.h"
#include "shell.h"
#include "vga.h"

void kmain(void) {
    vga_init();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_clear();

    vga_puts("OsU 0.1\n");
    vga_puts("Booting kernel...\n");

    idt_init();
    keyboard_init();
    memory_init();

    __asm__ volatile("sti");

    vga_puts("Ready.\n\n");
    shell_run();

    for (;;) {
        __asm__ volatile("hlt");
    }
}
