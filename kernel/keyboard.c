#include "keyboard.h"

#include <stdint.h>

#include "idt.h"
#include "io.h"
#include "vga.h"

#define KBD_BUF_SIZE 256

static volatile char kbd_buf[KBD_BUF_SIZE];
static volatile uint32_t kbd_head;
static volatile uint32_t kbd_tail;
static uint8_t shift_down;

static const char map_normal[128] = {
    0, 27, '1', '2', '3', '4', '5', '6',
    '7', '8', '9', '0', '-', '=', '\b', '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
    'o', 'p', '[', ']', '\n', 0, 'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
    '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

static const char map_shift[128] = {
    0, 27, '!', '@', '#', '$', '%', '^',
    '&', '*', '(', ')', '_', '+', '\b', '\t',
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
    'O', 'P', '{', '}', '\n', 0, 'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
    '"', '~', 0, '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', '<', '>', '?', 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

static void enqueue_char(char c) {
    uint32_t next = (kbd_head + 1) % KBD_BUF_SIZE;
    if (next == kbd_tail) {
        return;
    }
    kbd_buf[kbd_head] = c;
    kbd_head = next;
}

static int dequeue_char(char *out) {
    if (kbd_head == kbd_tail) {
        return 0;
    }
    *out = kbd_buf[kbd_tail];
    kbd_tail = (kbd_tail + 1) % KBD_BUF_SIZE;
    return 1;
}

static uint8_t extended_key = 0;

static void keyboard_irq_handler(registers_t *r) {
    (void)r;
    uint8_t scancode = inb(0x60);

    /* Handle extended key prefix */
    if (scancode == 0xE0) {
        extended_key = 1;
        return;
    }

    /* Handle shift keys */
    if (scancode == 0x2A || scancode == 0x36) {
        shift_down = 1;
        extended_key = 0;
        return;
    }
    if (scancode == 0xAA || scancode == 0xB6) {
        shift_down = 0;
        extended_key = 0;
        return;
    }

    /* Ignore key releases */
    if (scancode & 0x80) {
        extended_key = 0;
        return;
    }

    char ch = 0;

    if (extended_key) {
        /* Extended key scancodes */
        switch (scancode) {
            case 0x48: ch = KEY_UP; break;      /* Up arrow */
            case 0x50: ch = KEY_DOWN; break;    /* Down arrow */
            case 0x4B: ch = KEY_LEFT; break;    /* Left arrow */
            case 0x4D: ch = KEY_RIGHT; break;   /* Right arrow */
            case 0x47: ch = KEY_HOME; break;    /* Home */
            case 0x4F: ch = KEY_END; break;     /* End */
            case 0x49: ch = KEY_PGUP; break;    /* Page Up */
            case 0x51: ch = KEY_PGDN; break;    /* Page Down */
            case 0x53: ch = KEY_DELETE; break;  /* Delete */
        }
        extended_key = 0;
    } else {
        ch = shift_down ? map_shift[scancode] : map_normal[scancode];
    }

    if (ch) {
        enqueue_char(ch);
    }
}

void keyboard_init(void) {
    kbd_head = 0;
    kbd_tail = 0;
    shift_down = 0;
    extended_key = 0;
    idt_register_handler(33, keyboard_irq_handler);
}

/* Flush any pending keyboard input */
void keyboard_flush(void) {
    kbd_head = 0;
    kbd_tail = 0;
    extended_key = 0;
}

char keyboard_getchar(void) {
    char c;
    while (!dequeue_char(&c)) {
        /* Enable interrupts and halt atomically to avoid race condition.
         * Without sti before hlt, an interrupt could arrive after we check
         * the buffer but before we halt, causing us to sleep forever. */
        __asm__ volatile("sti; hlt");
    }
    return c;
}

void keyboard_readline(char *buf, size_t max_len) {
    size_t i = 0;

    if (max_len == 0) {
        return;
    }

    while (1) {
        char c = keyboard_getchar();
        if (c == '\r') {
            continue;
        }
        if (c == '\n') {
            vga_putc('\n');
            buf[i] = '\0';
            return;
        }
        if (c == '\b') {
            if (i > 0) {
                i--;
                vga_putc('\b');
            }
            continue;
        }
        if (i + 1 < max_len && c >= 32 && c <= 126) {
            buf[i++] = c;
            vga_putc(c);
        }
    }
}
