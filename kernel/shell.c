#include "shell.h"

#include <stdint.h>

#include "keyboard.h"
#include "lang.h"
#include "memory.h"
#include "io.h"
#include "string.h"
#include "vga.h"

#define SHELL_LINE_MAX 128

static int starts_with(const char *s, const char *prefix) {
    while (*prefix) {
        if (*s != *prefix) {
            return 0;
        }
        s++;
        prefix++;
    }
    return 1;
}

static void cmd_help(void) {
    vga_puts("Commands:\n");
    vga_puts("  help      show commands\n");
    vga_puts("  clear     clear screen\n");
    vga_puts("  echo X    print text\n");
    vga_puts("  mem       heap stats\n");
    vga_puts("  lang      tiny BASIC REPL\n");
    vga_puts("  reboot    reboot machine\n");
}

static void cmd_mem(void) {
    uintptr_t start = memory_heap_start();
    uintptr_t end = memory_heap_end();
    uintptr_t used = memory_heap_used();

    vga_puts("heap start: ");
    vga_print_hex((uint32_t)start);
    vga_putc('\n');

    vga_puts("heap end:   ");
    vga_print_hex((uint32_t)end);
    vga_putc('\n');

    vga_puts("heap used:  ");
    vga_print_dec((uint32_t)used);
    vga_puts(" bytes\n");
}

static void cmd_reboot(void) {
    vga_puts("Rebooting...\n");
    __asm__ volatile("cli");
    while (inb(0x64) & 0x02) {
    }
    outb(0x64, 0xFE);
    for (;;) {
        __asm__ volatile("hlt");
    }
}

void shell_run(void) {
    char line[SHELL_LINE_MAX];

    vga_puts("Type 'help' for commands.\n");
    for (;;) {
        vga_puts("mini> ");
        keyboard_readline(line, sizeof(line));

        if (line[0] == '\0') {
            continue;
        }

        if (strcmp(line, "help") == 0) {
            cmd_help();
        } else if (strcmp(line, "clear") == 0) {
            vga_clear();
        } else if (starts_with(line, "echo ")) {
            vga_puts(line + 5);
            vga_putc('\n');
        } else if (strcmp(line, "mem") == 0) {
            cmd_mem();
        } else if (strcmp(line, "lang") == 0) {
            lang_repl();
        } else if (strcmp(line, "reboot") == 0) {
            cmd_reboot();
        } else {
            vga_puts("Unknown command. Try 'help'.\n");
        }
    }
}
