#include "script.h"

#include <stddef.h>

#include "keyboard.h"
#include "string.h"
#include "vfs.h"
#include "vga.h"

#define SCRIPT_LINE_MAX 128

/* Forward declaration - implemented in shell.c */
/* We need to call shell commands from script, so we'll call the dispatch directly */
static int execute_line(const char *line);

/* Simple command executor - duplicates logic from shell for now */
static int execute_line(const char *line) {
    /* Skip whitespace */
    while (*line == ' ' || *line == '\t') {
        line++;
    }

    /* Skip empty lines and comments */
    if (*line == '\0' || *line == '#') {
        return 0;
    }

    /* Parse command and args */
    static char cmd_buf[SCRIPT_LINE_MAX];
    size_t i = 0;
    while (line[i] && i < SCRIPT_LINE_MAX - 1) {
        cmd_buf[i] = line[i];
        i++;
    }
    cmd_buf[i] = '\0';

    /* Find command end */
    char *cmd = cmd_buf;
    char *args = cmd_buf;
    while (*args && *args != ' ' && *args != '\t') {
        args++;
    }
    if (*args) {
        *args++ = '\0';
        while (*args == ' ' || *args == '\t') {
            args++;
        }
    }

    /* Execute known commands */
    if (strcmp(cmd, "echo") == 0) {
        vga_puts(args);
        vga_putc('\n');
    } else if (strcmp(cmd, "touch") == 0) {
        if (vfs_touch(args) != 0) {
            vga_puts("touch failed\n");
        }
    } else if (strcmp(cmd, "rm") == 0) {
        if (vfs_remove(args) != 0) {
            vga_puts("rm failed\n");
        }
    } else if (strcmp(cmd, "write") == 0) {
        /* Parse filename and text */
        char *name = args;
        char *text = args;
        while (*text && *text != ' ' && *text != '\t') {
            text++;
        }
        if (*text) {
            *text++ = '\0';
            while (*text == ' ' || *text == '\t') {
                text++;
            }
        }
        if (vfs_write(name, text) != 0) {
            vga_puts("write failed\n");
        }
    } else if (strcmp(cmd, "append") == 0) {
        char *name = args;
        char *text = args;
        while (*text && *text != ' ' && *text != '\t') {
            text++;
        }
        if (*text) {
            *text++ = '\0';
            while (*text == ' ' || *text == '\t') {
                text++;
            }
        }
        if (vfs_append(name, text) != 0) {
            vga_puts("append failed\n");
        }
    } else if (strcmp(cmd, "cat") == 0) {
        size_t len;
        const char *data = vfs_read_ptr(args, &len);
        if (data) {
            vga_puts(data);
            if (len == 0 || data[len - 1] != '\n') {
                vga_putc('\n');
            }
        } else {
            vga_puts("file not found\n");
        }
    } else if (strcmp(cmd, "mkdir") == 0) {
        if (vfs_mkdir(args) != 0) {
            vga_puts("mkdir failed\n");
        }
    } else if (strcmp(cmd, "rmdir") == 0) {
        if (vfs_rmdir(args) != 0) {
            vga_puts("rmdir failed\n");
        }
    } else if (strcmp(cmd, "cd") == 0) {
        if (*args == '\0') {
            vfs_chdir("/");
        } else if (vfs_chdir(args) != 0) {
            vga_puts("directory not found\n");
        }
    } else if (strcmp(cmd, "pwd") == 0) {
        vga_puts(vfs_getcwd());
        vga_putc('\n');
    } else if (strcmp(cmd, "clear") == 0) {
        vga_clear();
    } else if (strcmp(cmd, "ls") == 0) {
        const char *name;
        size_t len;
        int is_dir;
        size_t idx = 0;
        while (vfs_list_dir_entry(idx, &name, &len, &is_dir)) {
            vga_puts(name);
            if (is_dir) {
                vga_puts("  <DIR>\n");
            } else {
                vga_puts("  ");
                vga_print_dec((uint32_t)len);
                vga_puts("b\n");
            }
            idx++;
        }
    } else if (cmd[0] != '\0') {
        vga_puts("unknown command: ");
        vga_puts(cmd);
        vga_putc('\n');
        return -1;
    }

    return 0;
}

int script_run(const char *filename) {
    if (!filename || *filename == '\0') {
        return -1;
    }

    /* Read file content */
    size_t file_len;
    const char *content = vfs_read_ptr(filename, &file_len);
    if (!content) {
        vga_puts("script not found: ");
        vga_puts(filename);
        vga_putc('\n');
        return -1;
    }

    /* Process line by line */
    const char *p = content;
    const char *end = content + file_len;
    char line[SCRIPT_LINE_MAX];

    while (p < end) {
        /* Extract one line */
        size_t len = 0;
        while (p < end && *p != '\n' && len < SCRIPT_LINE_MAX - 1) {
            line[len++] = *p++;
        }
        line[len] = '\0';

        /* Skip newline */
        if (p < end && *p == '\n') {
            p++;
        }

        /* Execute line if not empty/comment */
        execute_line(line);
    }

    return 0;
}
