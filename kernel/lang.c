#include "lang.h"

#include <stddef.h>

#include "keyboard.h"
#include "string.h"
#include "vga.h"

#define MAX_LINE 96
#define MAX_PROG_LINES 64

typedef struct {
    int line_no;
    char code[MAX_LINE];
} prog_line_t;

static prog_line_t prog[MAX_PROG_LINES];
static int prog_count;
static int vars[26];

typedef struct {
    const char *s;
    size_t i;
} parser_t;

static int is_digit(char c) { return c >= '0' && c <= '9'; }
static int is_alpha(char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }

static char upper(char c) {
    if (c >= 'a' && c <= 'z') {
        return (char)(c - 32);
    }
    return c;
}

static void skip_ws(parser_t *p) {
    while (p->s[p->i] == ' ' || p->s[p->i] == '\t') {
        p->i++;
    }
}

static int parse_expr(parser_t *p);

static int parse_factor(parser_t *p) {
    skip_ws(p);
    char c = p->s[p->i];

    if (c == '(') {
        p->i++;
        int v = parse_expr(p);
        skip_ws(p);
        if (p->s[p->i] == ')') {
            p->i++;
        }
        return v;
    }

    if (is_alpha(c)) {
        int idx = upper(c) - 'A';
        p->i++;
        if (idx >= 0 && idx < 26) {
            return vars[idx];
        }
        return 0;
    }

    int sign = 1;
    if (c == '-') {
        sign = -1;
        p->i++;
    }

    int value = 0;
    while (is_digit(p->s[p->i])) {
        value = value * 10 + (p->s[p->i] - '0');
        p->i++;
    }
    return value * sign;
}

static int parse_term(parser_t *p) {
    int v = parse_factor(p);
    while (1) {
        skip_ws(p);
        char op = p->s[p->i];
        if (op != '*' && op != '/') {
            break;
        }
        p->i++;
        int rhs = parse_factor(p);
        if (op == '*') {
            v *= rhs;
        } else {
            if (rhs == 0) {
                vga_puts("ERR: div by zero\n");
                return 0;
            }
            v /= rhs;
        }
    }
    return v;
}

static int parse_expr(parser_t *p) {
    int v = parse_term(p);
    while (1) {
        skip_ws(p);
        char op = p->s[p->i];
        if (op != '+' && op != '-') {
            break;
        }
        p->i++;
        int rhs = parse_term(p);
        if (op == '+') {
            v += rhs;
        } else {
            v -= rhs;
        }
    }
    return v;
}

static int starts_with_kw(const char *s, const char *kw) {
    size_t i = 0;
    while (kw[i]) {
        if (upper(s[i]) != kw[i]) {
            return 0;
        }
        i++;
    }
    return 1;
}

static void copy_line(char *dst, const char *src) {
    size_t i = 0;
    while (src[i] && i + 1 < MAX_LINE) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static void add_program_line(int line_no, const char *code) {
    int pos = 0;
    while (pos < prog_count && prog[pos].line_no < line_no) {
        pos++;
    }

    if (pos < prog_count && prog[pos].line_no == line_no) {
        copy_line(prog[pos].code, code);
        return;
    }

    if (prog_count >= MAX_PROG_LINES) {
        vga_puts("ERR: program full\n");
        return;
    }

    for (int i = prog_count; i > pos; i--) {
        prog[i] = prog[i - 1];
    }
    prog[pos].line_no = line_no;
    copy_line(prog[pos].code, code);
    prog_count++;
}

static void exec_line(const char *line);

static void cmd_run(void) {
    for (int i = 0; i < prog_count; i++) {
        exec_line(prog[i].code);
    }
}

static void cmd_list(void) {
    for (int i = 0; i < prog_count; i++) {
        vga_print_dec((uint32_t)prog[i].line_no);
        vga_putc(' ');
        vga_puts(prog[i].code);
        vga_putc('\n');
    }
}

static void exec_line(const char *line) {
    if (starts_with_kw(line, "PRINT")) {
        const char *arg = line + 5;
        while (*arg == ' ') {
            arg++;
        }
        if (*arg == '"') {
            arg++;
            while (*arg && *arg != '"') {
                vga_putc(*arg++);
            }
            vga_putc('\n');
            return;
        }
        parser_t p = {arg, 0};
        int result = parse_expr(&p);
        vga_print_dec((uint32_t)result);
        vga_putc('\n');
        return;
    }

    if (starts_with_kw(line, "LET")) {
        const char *arg = line + 3;
        while (*arg == ' ') {
            arg++;
        }
        if (!is_alpha(*arg)) {
            vga_puts("ERR: LET var\n");
            return;
        }
        int idx = upper(*arg) - 'A';
        arg++;
        while (*arg == ' ') {
            arg++;
        }
        if (*arg != '=') {
            vga_puts("ERR: expected =\n");
            return;
        }
        arg++;
        parser_t p = {arg, 0};
        vars[idx] = parse_expr(&p);
        return;
    }

    if (starts_with_kw(line, "RUN")) {
        cmd_run();
        return;
    }

    if (starts_with_kw(line, "LIST")) {
        cmd_list();
        return;
    }

    if (starts_with_kw(line, "NEW")) {
        prog_count = 0;
        for (int i = 0; i < 26; i++) {
            vars[i] = 0;
        }
        vga_puts("OK\n");
        return;
    }

    if (starts_with_kw(line, "VARS")) {
        for (int i = 0; i < 26; i++) {
            vga_putc((char)('A' + i));
            vga_putc('=');
            vga_print_dec((uint32_t)vars[i]);
            vga_putc(' ');
            if ((i % 6) == 5) {
                vga_putc('\n');
            }
        }
        vga_putc('\n');
        return;
    }

    vga_puts("ERR: unknown\n");
}

void lang_repl(void) {
    char line[MAX_LINE];

    vga_puts("\nTiny BASIC subset\n");
    vga_puts("PRINT, LET, RUN, LIST, NEW, VARS\n");
    vga_puts("Type EXIT to return\n\n");

    while (1) {
        vga_puts("BASIC> ");
        keyboard_readline(line, sizeof(line));

        if (strcmp(line, "EXIT") == 0 || strcmp(line, "exit") == 0) {
            vga_puts("Leaving BASIC\n");
            return;
        }

        size_t i = 0;
        int line_no = -1;
        if (is_digit(line[0])) {
            line_no = 0;
            while (is_digit(line[i])) {
                line_no = line_no * 10 + (line[i] - '0');
                i++;
            }
            while (line[i] == ' ') {
                i++;
            }
        }

        if (line_no >= 0) {
            add_program_line(line_no, &line[i]);
        } else {
            exec_line(line);
        }
    }
}
