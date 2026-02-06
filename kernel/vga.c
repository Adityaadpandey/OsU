#include "vga.h"

#include <stdarg.h>

#include "io.h"

#define VGA_MEM ((volatile uint16_t *)0xB8000)

static uint8_t cursor_x;
static uint8_t cursor_y;
static uint8_t color;

static inline uint16_t make_cell(char c, uint8_t col) {
    return (uint16_t)((uint8_t)c) | ((uint16_t)col << 8);
}

static void update_cursor(void) {
    uint16_t pos = (uint16_t)(cursor_y * VGA_WIDTH + cursor_x);
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

static void scroll_if_needed(void) {
    if (cursor_y < VGA_HEIGHT) {
        return;
    }

    for (size_t y = 1; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            VGA_MEM[(y - 1) * VGA_WIDTH + x] = VGA_MEM[y * VGA_WIDTH + x];
        }
    }
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        VGA_MEM[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = make_cell(' ', color);
    }
    cursor_y = VGA_HEIGHT - 1;
}

void vga_init(void) {
    cursor_x = 0;
    cursor_y = 0;
    color = (uint8_t)(VGA_COLOR_LIGHT_GREY | (VGA_COLOR_BLACK << 4));
    vga_clear();
}

void vga_clear(void) {
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_MEM[i] = make_cell(' ', color);
    }
    cursor_x = 0;
    cursor_y = 0;
    update_cursor();
}

void vga_set_color(vga_color_t fg, vga_color_t bg) {
    color = (uint8_t)(fg | (bg << 4));
}

void vga_putc(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            VGA_MEM[cursor_y * VGA_WIDTH + cursor_x] = make_cell(' ', color);
        }
    } else {
        VGA_MEM[cursor_y * VGA_WIDTH + cursor_x] = make_cell(c, color);
        cursor_x++;
    }

    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
    scroll_if_needed();
    update_cursor();
}

void vga_puts(const char *str) {
    while (*str) {
        vga_putc(*str++);
    }
}

void vga_print_dec(uint32_t value) {
    char buf[11];
    int i = 10;
    buf[i] = '\0';
    if (value == 0) {
        vga_putc('0');
        return;
    }
    while (value && i > 0) {
        buf[--i] = (char)('0' + (value % 10));
        value /= 10;
    }
    vga_puts(&buf[i]);
}

void vga_print_hex(uint32_t value) {
    static const char *hex = "0123456789ABCDEF";
    vga_puts("0x");
    for (int i = 7; i >= 0; i--) {
        uint8_t nibble = (uint8_t)((value >> (i * 4)) & 0xF);
        vga_putc(hex[nibble]);
    }
}

void vga_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    while (*fmt) {
        if (*fmt != '%') {
            vga_putc(*fmt++);
            continue;
        }

        fmt++;
        if (*fmt == '%') {
            vga_putc('%');
        } else if (*fmt == 's') {
            const char *s = va_arg(args, const char *);
            vga_puts(s ? s : "(null)");
        } else if (*fmt == 'c') {
            char c = (char)va_arg(args, int);
            vga_putc(c);
        } else if (*fmt == 'd') {
            int v = va_arg(args, int);
            if (v < 0) {
                vga_putc('-');
                v = -v;
            }
            vga_print_dec((uint32_t)v);
        } else if (*fmt == 'u') {
            uint32_t v = va_arg(args, uint32_t);
            vga_print_dec(v);
        } else if (*fmt == 'x') {
            uint32_t v = va_arg(args, uint32_t);
            vga_print_hex(v);
        } else {
            vga_putc('%');
            vga_putc(*fmt);
        }
        fmt++;
    }
    va_end(args);
}
