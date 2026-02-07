#include "cospy.h"

#include <stddef.h>
#include <stdint.h>

#include "keyboard.h"
#include "memory.h"
#include "string.h"
#include "vfs.h"
#include "vga.h"

/* ==================== Configuration ==================== */
#define COSPY_LINE_MAX 128
#define COSPY_MAX_VARS 64
#define COSPY_MAX_FUNCS 16
#define COSPY_STACK_MAX 32
#define COSPY_MAX_FUNC_LINES 32

/* ==================== Token Types ==================== */
typedef enum {
    TOK_EOF,
    TOK_NUMBER,
    TOK_STRING,
    TOK_IDENT,
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_PERCENT,
    TOK_EQ,
    TOK_EQEQ,
    TOK_NE,
    TOK_LT,
    TOK_GT,
    TOK_LE,
    TOK_GE,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_COLON,
    TOK_COMMA,
    TOK_AND,
    TOK_OR,
    TOK_NOT
} token_type_t;

/* ==================== Variables ==================== */
typedef struct {
    char name[16];
    int32_t value;
} variable_t;

static variable_t variables[COSPY_MAX_VARS];
static int var_count;

/* ==================== Functions ==================== */
typedef struct {
    char name[16];
    char lines[COSPY_MAX_FUNC_LINES][COSPY_LINE_MAX];
    int line_count;
} function_t;

static function_t functions[COSPY_MAX_FUNCS];
static int func_count;

/* ==================== Execution State ==================== */
static const char *cursor;
static token_type_t current_token;
static int32_t token_number;
static char token_string[64];
static char token_ident[32];

static int in_if_false;        /* Skip lines inside false if block */
static int if_depth;           /* Track nested ifs */
static int in_while;           /* Inside while loop */
static int while_depth;        /* Track nested whiles */
static char while_cond[COSPY_LINE_MAX]; /* While condition */

static int defining_function;  /* Currently defining a function */
static int current_func_idx;   /* Index of function being defined */

/* ==================== Tokenizer ==================== */
static void skip_whitespace(void) {
    while (*cursor == ' ' || *cursor == '\t') {
        cursor++;
    }
}

static int is_digit(char c) {
    return c >= '0' && c <= '9';
}

static int is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static int is_alnum(char c) {
    return is_alpha(c) || is_digit(c);
}

static void next_token(void) {
    skip_whitespace();

    if (*cursor == '\0' || *cursor == '#') {
        current_token = TOK_EOF;
        return;
    }

    /* Number */
    if (is_digit(*cursor) || (*cursor == '-' && is_digit(cursor[1]))) {
        int sign = 1;
        if (*cursor == '-') {
            sign = -1;
            cursor++;
        }
        token_number = 0;
        while (is_digit(*cursor)) {
            token_number = token_number * 10 + (*cursor - '0');
            cursor++;
        }
        token_number *= sign;
        current_token = TOK_NUMBER;
        return;
    }

    /* String */
    if (*cursor == '"' || *cursor == '\'') {
        char quote = *cursor++;
        int i = 0;
        while (*cursor && *cursor != quote && i < 63) {
            token_string[i++] = *cursor++;
        }
        token_string[i] = '\0';
        if (*cursor == quote) cursor++;
        current_token = TOK_STRING;
        return;
    }

    /* Identifier or keyword */
    if (is_alpha(*cursor)) {
        int i = 0;
        while (is_alnum(*cursor) && i < 31) {
            token_ident[i++] = *cursor++;
        }
        token_ident[i] = '\0';

        if (strcmp(token_ident, "and") == 0) {
            current_token = TOK_AND;
        } else if (strcmp(token_ident, "or") == 0) {
            current_token = TOK_OR;
        } else if (strcmp(token_ident, "not") == 0) {
            current_token = TOK_NOT;
        } else {
            current_token = TOK_IDENT;
        }
        return;
    }

    /* Operators */
    switch (*cursor) {
        case '+': cursor++; current_token = TOK_PLUS; return;
        case '-': cursor++; current_token = TOK_MINUS; return;
        case '*': cursor++; current_token = TOK_STAR; return;
        case '/': cursor++; current_token = TOK_SLASH; return;
        case '%': cursor++; current_token = TOK_PERCENT; return;
        case '(': cursor++; current_token = TOK_LPAREN; return;
        case ')': cursor++; current_token = TOK_RPAREN; return;
        case ':': cursor++; current_token = TOK_COLON; return;
        case ',': cursor++; current_token = TOK_COMMA; return;
        case '=':
            cursor++;
            if (*cursor == '=') {
                cursor++;
                current_token = TOK_EQEQ;
            } else {
                current_token = TOK_EQ;
            }
            return;
        case '!':
            cursor++;
            if (*cursor == '=') {
                cursor++;
                current_token = TOK_NE;
            } else {
                current_token = TOK_NOT;
            }
            return;
        case '<':
            cursor++;
            if (*cursor == '=') {
                cursor++;
                current_token = TOK_LE;
            } else {
                current_token = TOK_GT; /* Actually LT */
                current_token = TOK_LT;
            }
            return;
        case '>':
            cursor++;
            if (*cursor == '=') {
                cursor++;
                current_token = TOK_GE;
            } else {
                current_token = TOK_GT;
            }
            return;
        default:
            cursor++;
            current_token = TOK_EOF;
            return;
    }
}

/* ==================== Variables ==================== */
static int32_t *get_var(const char *name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            return &variables[i].value;
        }
    }
    return 0;
}

static int32_t *set_var(const char *name, int32_t value) {
    int32_t *v = get_var(name);
    if (v) {
        *v = value;
        return v;
    }
    if (var_count >= COSPY_MAX_VARS) {
        return 0;
    }
    strcpy(variables[var_count].name, name);
    variables[var_count].value = value;
    return &variables[var_count++].value;
}

/* ==================== Expression Parser ==================== */
static int32_t parse_expr(void);

static int32_t parse_primary(void) {
    if (current_token == TOK_NUMBER) {
        int32_t val = token_number;
        next_token();
        return val;
    }

    if (current_token == TOK_STRING) {
        /* Strings evaluate to their length for now */
        int32_t len = (int32_t)strlen(token_string);
        next_token();
        return len;
    }

    if (current_token == TOK_IDENT) {
        char name[32];
        strcpy(name, token_ident);
        next_token();

        /* Check for function call */
        if (current_token == TOK_LPAREN) {
            next_token(); /* consume ( */
            /* For now, skip arguments */
            while (current_token != TOK_RPAREN && current_token != TOK_EOF) {
                next_token();
            }
            if (current_token == TOK_RPAREN) {
                next_token();
            }

            /* Built-in input() */
            if (strcmp(name, "input") == 0) {
                char buf[64];
                keyboard_readline(buf, sizeof(buf));
                return atoi(buf);
            }

            return 0;
        }

        /* Variable lookup */
        int32_t *v = get_var(name);
        return v ? *v : 0;
    }

    if (current_token == TOK_LPAREN) {
        next_token();
        int32_t val = parse_expr();
        if (current_token == TOK_RPAREN) {
            next_token();
        }
        return val;
    }

    if (current_token == TOK_MINUS) {
        next_token();
        return -parse_primary();
    }

    if (current_token == TOK_NOT) {
        next_token();
        return !parse_primary();
    }

    return 0;
}

static int32_t parse_term(void) {
    int32_t left = parse_primary();

    while (current_token == TOK_STAR || current_token == TOK_SLASH || current_token == TOK_PERCENT) {
        token_type_t op = current_token;
        next_token();
        int32_t right = parse_primary();

        if (op == TOK_STAR) {
            left = left * right;
        } else if (op == TOK_SLASH) {
            if (right != 0) {
                left = left / right;
            } else {
                vga_puts("division by zero\n");
                left = 0;
            }
        } else {
            if (right != 0) {
                left = left % right;
            } else {
                left = 0;
            }
        }
    }

    return left;
}

static int32_t parse_additive(void) {
    int32_t left = parse_term();

    while (current_token == TOK_PLUS || current_token == TOK_MINUS) {
        token_type_t op = current_token;
        next_token();
        int32_t right = parse_term();

        if (op == TOK_PLUS) {
            left = left + right;
        } else {
            left = left - right;
        }
    }

    return left;
}

static int32_t parse_comparison(void) {
    int32_t left = parse_additive();

    while (current_token == TOK_LT || current_token == TOK_GT ||
           current_token == TOK_LE || current_token == TOK_GE ||
           current_token == TOK_EQEQ || current_token == TOK_NE) {
        token_type_t op = current_token;
        next_token();
        int32_t right = parse_additive();

        switch (op) {
            case TOK_LT: left = left < right; break;
            case TOK_GT: left = left > right; break;
            case TOK_LE: left = left <= right; break;
            case TOK_GE: left = left >= right; break;
            case TOK_EQEQ: left = left == right; break;
            case TOK_NE: left = left != right; break;
            default: break;
        }
    }

    return left;
}

static int32_t parse_and(void) {
    int32_t left = parse_comparison();

    while (current_token == TOK_AND) {
        next_token();
        int32_t right = parse_comparison();
        left = left && right;
    }

    return left;
}

static int32_t parse_expr(void) {
    int32_t left = parse_and();

    while (current_token == TOK_OR) {
        next_token();
        int32_t right = parse_and();
        left = left || right;
    }

    return left;
}

/* ==================== Statement Execution ==================== */
static int execute_statement(const char *line);

static void run_function(const char *name) {
    for (int i = 0; i < func_count; i++) {
        if (strcmp(functions[i].name, name) == 0) {
            for (int j = 0; j < functions[i].line_count; j++) {
                execute_statement(functions[i].lines[j]);
            }
            return;
        }
    }
    vga_puts("undefined function: ");
    vga_puts(name);
    vga_putc('\n');
}

static int execute_statement(const char *line) {
    /* Skip whitespace */
    while (*line == ' ' || *line == '\t') {
        line++;
    }

    /* Empty line or comment */
    if (*line == '\0' || *line == '#') {
        return 0;
    }

    /* Check for function definition end */
    if (defining_function) {
        if (strncmp(line, "enddef", 6) == 0) {
            defining_function = 0;
            return 0;
        }
        /* Add line to function */
        if (current_func_idx >= 0 && current_func_idx < COSPY_MAX_FUNCS) {
            function_t *f = &functions[current_func_idx];
            if (f->line_count < COSPY_MAX_FUNC_LINES) {
                strcpy(f->lines[f->line_count++], line);
            }
        }
        return 0;
    }

    /* Check keywords */
    if (strncmp(line, "exit", 4) == 0 || strncmp(line, "quit", 4) == 0) {
        return -1; /* Signal exit */
    }

    /* Handle if/else/endif */
    if (strncmp(line, "endif", 5) == 0) {
        if (if_depth > 0) {
            if_depth--;
            if (if_depth == 0) {
                in_if_false = 0;
            }
        }
        return 0;
    }

    if (strncmp(line, "else", 4) == 0 && (line[4] == ':' || line[4] == '\0' || line[4] == ' ')) {
        if (if_depth == 1) {
            in_if_false = !in_if_false;
        }
        return 0;
    }

    if (strncmp(line, "if ", 3) == 0) {
        if_depth++;
        if (!in_if_false) {
            cursor = line + 3;
            next_token();
            int32_t cond = parse_expr();
            if (!cond) {
                in_if_false = 1;
            }
        }
        return 0;
    }

    /* Skip if we're in a false block */
    if (in_if_false) {
        return 0;
    }

    /* Handle while/endwhile */
    if (strncmp(line, "endwhile", 8) == 0) {
        /* This is handled in the REPL loop */
        return 0;
    }

    if (strncmp(line, "while ", 6) == 0) {
        /* Store condition for loop */
        strcpy(while_cond, line + 6);
        while_depth++;
        return 0;
    }

    /* Function definition */
    if (strncmp(line, "def ", 4) == 0) {
        if (func_count >= COSPY_MAX_FUNCS) {
            vga_puts("too many functions\n");
            return 0;
        }
        /* Parse function name */
        const char *p = line + 4;
        int i = 0;
        while (*p && *p != '(' && i < 15) {
            functions[func_count].name[i++] = *p++;
        }
        functions[func_count].name[i] = '\0';
        functions[func_count].line_count = 0;
        current_func_idx = func_count;
        func_count++;
        defining_function = 1;
        return 0;
    }

    /* print statement */
    if (strncmp(line, "print", 5) == 0 && line[5] == '(') {
        cursor = line + 6;
        next_token();

        if (current_token == TOK_STRING) {
            vga_puts(token_string);
            vga_putc('\n');
        } else {
            int32_t val = parse_expr();
            vga_print_dec((uint32_t)val);
            vga_putc('\n');
        }
        return 0;
    }

    /* Function call (no assignment) */
    cursor = line;
    next_token();
    if (current_token == TOK_IDENT) {
        char name[32];
        strcpy(name, token_ident);
        next_token();

        if (current_token == TOK_LPAREN) {
            /* Check for user-defined function */
            for (int i = 0; i < func_count; i++) {
                if (strcmp(functions[i].name, name) == 0) {
                    run_function(name);
                    return 0;
                }
            }
            /* Built-in functions handled in parse_primary */
            return 0;
        }

        /* Assignment: x = expr */
        if (current_token == TOK_EQ) {
            next_token();
            int32_t val = parse_expr();
            set_var(name, val);
            return 0;
        }
    }

    return 0;
}

/* ==================== REPL ==================== */
static void cospy_init(void) {
    var_count = 0;
    func_count = 0;
    in_if_false = 0;
    if_depth = 0;
    in_while = 0;
    while_depth = 0;
    defining_function = 0;
}

void cospy_repl(void) {
    char line[COSPY_LINE_MAX];
    char while_body[16][COSPY_LINE_MAX];
    int while_body_count = 0;

    cospy_init();

    vga_puts("\nCosyPy v1.0 - Python-like interpreter\n");
    vga_puts("Type 'exit' to return to shell\n\n");

    while (1) {
        vga_puts("py> ");
        keyboard_readline(line, sizeof(line));

        /* Handle while loop body collection */
        if (while_depth > 0 && strncmp(line, "endwhile", 8) != 0) {
            if (while_body_count < 16) {
                strcpy(while_body[while_body_count++], line);
            }
            /* Check for nested while */
            if (strncmp(line, "while ", 6) == 0) {
                while_depth++;
            }
            continue;
        }

        /* Execute while loop */
        if (while_depth > 0 && strncmp(line, "endwhile", 8) == 0) {
            while_depth--;
            if (while_depth == 0) {
                /* Execute the loop */
                int max_iter = 1000; /* Prevent infinite loops */
                while (max_iter-- > 0) {
                    /* Evaluate condition */
                    cursor = while_cond;
                    next_token();
                    if (!parse_expr()) {
                        break;
                    }
                    /* Execute body */
                    for (int i = 0; i < while_body_count; i++) {
                        if (execute_statement(while_body[i]) < 0) {
                            break;
                        }
                    }
                }
                while_body_count = 0;
            }
            continue;
        }

        int result = execute_statement(line);
        if (result < 0) {
            vga_puts("Leaving CosyPy\n");
            break;
        }
    }
}

int cospy_run_file(const char *filename) {
    if (!filename || *filename == '\0') {
        return -1;
    }

    size_t file_len;
    const char *content = vfs_read_ptr(filename, &file_len);
    if (!content) {
        vga_puts("file not found: ");
        vga_puts(filename);
        vga_putc('\n');
        return -1;
    }

    cospy_init();

    /* Collect all lines first for proper loop handling */
    char lines[64][COSPY_LINE_MAX];
    int line_count = 0;

    const char *p = content;
    const char *end = content + file_len;

    while (p < end && line_count < 64) {
        size_t len = 0;
        while (p < end && *p != '\n' && len < COSPY_LINE_MAX - 1) {
            lines[line_count][len++] = *p++;
        }
        lines[line_count][len] = '\0';
        line_count++;
        if (p < end && *p == '\n') p++;
    }

    /* Execute lines with while loop support */
    char while_body[16][COSPY_LINE_MAX];
    int while_body_count = 0;

    for (int i = 0; i < line_count; i++) {
        const char *line = lines[i];

        /* Skip whitespace */
        while (*line == ' ' || *line == '\t') line++;

        /* Handle while loop body collection */
        if (while_depth > 0 && strncmp(line, "endwhile", 8) != 0) {
            if (while_body_count < 16) {
                strcpy(while_body[while_body_count++], line);
            }
            if (strncmp(line, "while ", 6) == 0) {
                while_depth++;
            }
            continue;
        }

        if (while_depth > 0 && strncmp(line, "endwhile", 8) == 0) {
            while_depth--;
            if (while_depth == 0) {
                int max_iter = 1000;
                while (max_iter-- > 0) {
                    cursor = while_cond;
                    next_token();
                    if (!parse_expr()) {
                        break;
                    }
                    for (int j = 0; j < while_body_count; j++) {
                        execute_statement(while_body[j]);
                    }
                }
                while_body_count = 0;
            }
            continue;
        }

        execute_statement(line);
    }

    return 0;
}
