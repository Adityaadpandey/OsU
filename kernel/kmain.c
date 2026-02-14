#include "idt.h"
#include "keyboard.h"
#include "disk.h"
#include "gfxcon.h"
#include "memory.h"
#include "shell.h"
#include "syscall.h"
#include "tss.h"
#include "vfs.h"
#include "vga.h"
#include "vesa.h"
#include "v86.h"
#include "timer.h"
#include "process.h"

void kmain(void) {
    /* Initialize VESA first (before any vga_* calls) */
    vesa_init();

    /* Initialize graphics console if VESA is available */
    if (vesa_enabled) {
        gfxcon_init();
    }

    /* Now init VGA (will redirect to gfxcon if active) */
    vga_init();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_clear();

    vga_puts("OsU 0.1\n");
    if (vesa_enabled) {
        vga_puts("Graphics: ");
        vga_print_dec(vesa_width);
        vga_puts("x");
        vga_print_dec(vesa_height);
        vga_puts("x");
        vga_print_dec(vesa_bpp);
        vga_puts("\n");
    }
    vga_puts("Booting kernel...\n");

    idt_init();
    keyboard_init();
    memory_init();
    disk_init();
    vfs_init();

    /* Initialize timer (100Hz = 10ms per tick) */
    timer_init(100);

    /* Initialize process management */
    process_init();

    /* Initialize TSS for ring transitions */
    tss_init();

    /* Initialize syscall interface */
    syscall_init();

    /* Initialize V86 mode for BIOS calls */
    v86_init();

    __asm__ volatile("sti");

    vga_puts("Ready.\n\n");
    shell_run();

    for (;;) {
        __asm__ volatile("hlt");
    }
}
