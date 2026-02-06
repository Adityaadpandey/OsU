#include "lang.h"

#include <stddef.h>
#include <stdint.h>

#include "keyboard.h"
#include "memory.h"
#include "string.h"
#include "vga.h"

#define LANG_LINE 96
#define STACK_MAX 128

static int32_t stack[STACK_MAX];
static int sp;

static int pop(int32_t *v) {
    if (sp <= 0) {
        return -1;
    }
    *v = stack[--sp];
    return 0;
}

static int push(int32_t v) {
    if (sp >= STACK_MAX) {
        return -1;
    }
    stack[sp++] = v;
    return 0;
}

static int is_number(const char *s) {
    if (*s == '-' || *s == '+') {
        s++;
    }
    if (*s == '\0') {
        return 0;
    }
    while (*s) {
        if (*s < '0' || *s > '9') {
            return 0;
        }
        s++;
    }
    return 1;
}

static void print_stack(void) {
    vga_puts("<");
    vga_print_dec((uint32_t)sp);
    vga_puts("> ");
    for (int i = 0; i < sp; i++) {
        vga_print_dec((uint32_t)stack[i]);
        vga_putc(' ');
    }
    vga_putc('\n');
}

static char *next_tok(char **p) {
    while (**p == ' ' || **p == '\t') {
        (*p)++;
    }
    if (**p == '\0') {
        return 0;
    }
    char *start = *p;
    while (**p && **p != ' ' && **p != '\t') {
        (*p)++;
    }
    if (**p) {
        **p = '\0';
        (*p)++;
    }
    return start;
}

void lang_repl(void) {
    char line[LANG_LINE];

    sp = 0;
    vga_puts("\nTiny Forth (interim for MicroPython prep)\n");
    vga_puts("Words: + - * / dup drop swap . .s mem clear words bye\n\n");

    while (1) {
        vga_puts("forth> ");
        keyboard_readline(line, sizeof(line));

        char *cur = line;
        char *tok;
        while ((tok = next_tok(&cur)) != 0) {
            if (strcmp(tok, "bye") == 0 || strcmp(tok, "exit") == 0) {
                vga_puts("Leaving forth\n");
                return;
            } else if (strcmp(tok, "+") == 0) {
                int32_t a, b;
                if (pop(&b) || pop(&a)) {
                    vga_puts("stack underflow\n");
                } else {
                    push(a + b);
                }
            } else if (strcmp(tok, "-") == 0) {
                int32_t a, b;
                if (pop(&b) || pop(&a)) {
                    vga_puts("stack underflow\n");
                } else {
                    push(a - b);
                }
            } else if (strcmp(tok, "*") == 0) {
                int32_t a, b;
                if (pop(&b) || pop(&a)) {
                    vga_puts("stack underflow\n");
                } else {
                    push(a * b);
                }
            } else if (strcmp(tok, "/") == 0) {
                int32_t a, b;
                if (pop(&b) || pop(&a)) {
                    vga_puts("stack underflow\n");
                } else if (b == 0) {
                    vga_puts("div by zero\n");
                    push(a);
                    push(b);
                } else {
                    push(a / b);
                }
            } else if (strcmp(tok, "dup") == 0) {
                if (sp <= 0 || push(stack[sp - 1])) {
                    vga_puts("stack error\n");
                }
            } else if (strcmp(tok, "drop") == 0) {
                int32_t tmp;
                if (pop(&tmp)) {
                    vga_puts("stack underflow\n");
                }
            } else if (strcmp(tok, "swap") == 0) {
                if (sp < 2) {
                    vga_puts("stack underflow\n");
                } else {
                    int32_t t = stack[sp - 1];
                    stack[sp - 1] = stack[sp - 2];
                    stack[sp - 2] = t;
                }
            } else if (strcmp(tok, ".") == 0) {
                int32_t v;
                if (pop(&v)) {
                    vga_puts("stack underflow\n");
                } else {
                    vga_print_dec((uint32_t)v);
                    vga_putc('\n');
                }
            } else if (strcmp(tok, ".s") == 0) {
                print_stack();
            } else if (strcmp(tok, "mem") == 0) {
                vga_puts("heap used: ");
                vga_print_dec((uint32_t)memory_heap_used());
                vga_puts(" bytes\n");
            } else if (strcmp(tok, "clear") == 0) {
                sp = 0;
            } else if (strcmp(tok, "words") == 0) {
                vga_puts("+ - * / dup drop swap . .s mem clear words bye\n");
            } else if (is_number(tok)) {
                if (push((int32_t)atoi(tok)) != 0) {
                    vga_puts("stack full\n");
                }
            } else {
                vga_puts("unknown word: ");
                vga_puts(tok);
                vga_putc('\n');
            }
        }
    }
}
