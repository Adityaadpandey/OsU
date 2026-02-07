#include "shell.h"

#include <stdint.h>

#include "editor.h"
#include "io.h"
#include "keyboard.h"
#include "lang.h"
#include "memory.h"
#include "string.h"
#include "vfs.h"
#include "vga.h"

#define SHELL_LINE_MAX 192
#define HISTORY_MAX 16

static char history[HISTORY_MAX][SHELL_LINE_MAX];
static size_t history_count;

static void save_history(const char *line) {
    if (line[0] == '\0') {
        return;
    }
    if (history_count < HISTORY_MAX) {
        strcpy(history[history_count++], line);
        return;
    }
    for (size_t i = 1; i < HISTORY_MAX; i++) {
        strcpy(history[i - 1], history[i]);
    }
    strcpy(history[HISTORY_MAX - 1], line);
}

static const char *skip_ws(const char *s) {
    while (*s == ' ' || *s == '\t') {
        s++;
    }
    return s;
}

static int split_cmd(char *line, char **cmd, char **args) {
    char *p = line;
    while (*p == ' ' || *p == '\t') {
        p++;
    }
    if (*p == '\0') {
        return 0;
    }

    *cmd = p;
    while (*p && *p != ' ' && *p != '\t') {
        p++;
    }
    if (*p == '\0') {
        *args = p;
        return 1;
    }
    *p++ = '\0';
    while (*p == ' ' || *p == '\t') {
        p++;
    }
    *args = p;
    return 1;
}

static void split_first_arg(char *args, char **a0, char **rest) {
    args = (char *)skip_ws(args);
    *a0 = args;
    while (*args && *args != ' ' && *args != '\t') {
        args++;
    }
    if (*args == '\0') {
        *rest = args;
        return;
    }
    *args++ = '\0';
    *rest = (char *)skip_ws(args);
}

static void cmd_help(void) {
    vga_puts("Commands:\n");
    vga_puts("  help                show commands\n");
    vga_puts("  clear               clear screen\n");
    vga_puts("  echo TEXT           print text\n");
    vga_puts("  mem                 heap stats\n");
    vga_puts("  history             command history\n");
    vga_puts("  lang                forth REPL\n");
    vga_puts("  python              CosyPy REPL\n");
    vga_puts("  run FILE.sh         run shell script\n");
    vga_puts("  pyrun FILE.py       run CosyPy script\n");
    vga_puts("  ls                  list files\n");
    vga_puts("  cat FILE            print file\n");
    vga_puts("  touch FILE          create file\n");
    vga_puts("  rm FILE             remove file\n");
    vga_puts("  write FILE TEXT     overwrite file\n");
    vga_puts("  append FILE TEXT    append to file\n");
    vga_puts("  edit FILE           vim-like editor\n");
    vga_puts("  pwd                 print working directory\n");
    vga_puts("  cd DIR              change directory\n");
    vga_puts("  mkdir DIR           create directory\n");
    vga_puts("  rmdir DIR           remove empty directory\n");
    vga_puts("  reboot              reboot machine\n");
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

static void cmd_history(void) {
    for (size_t i = 0; i < history_count; i++) {
        vga_print_dec((uint32_t)i);
        vga_puts(": ");
        vga_puts(history[i]);
        vga_putc('\n');
    }
}

static void cmd_ls(void) {
    const char *name;
    size_t len;
    int is_dir;
    size_t i = 0;

    while (vfs_list_dir_entry(i, &name, &len, &is_dir)) {
        vga_puts(name);
        if (is_dir) {
            vga_puts("  <DIR>\n");
        } else {
            vga_puts("  ");
            vga_print_dec((uint32_t)len);
            vga_puts("b\n");
        }
        i++;
    }
    if (i == 0) {
        vga_puts("(empty)\n");
    }
}

static void cmd_pwd(void) {
    vga_puts(vfs_getcwd());
    vga_putc('\n');
}

static void cmd_cd(char *args) {
    if (*args == '\0') {
        vfs_chdir("/");
        return;
    }
    if (vfs_chdir(args) != 0) {
        vga_puts("directory not found\n");
    }
}

static void cmd_mkdir(char *args) {
    if (*args == '\0') {
        vga_puts("usage: mkdir DIR\n");
        return;
    }
    if (vfs_mkdir(args) == 0) {
        vga_puts("ok\n");
    } else {
        vga_puts("mkdir failed\n");
    }
}

static void cmd_rmdir(char *args) {
    if (*args == '\0') {
        vga_puts("usage: rmdir DIR\n");
        return;
    }
    int r = vfs_rmdir(args);
    if (r == 0) {
        vga_puts("ok\n");
    } else if (r == -4) {
        vga_puts("directory not empty\n");
    } else {
        vga_puts("rmdir failed\n");
    }
}

static void cmd_cat(char *args) {
    if (*args == '\0') {
        vga_puts("usage: cat FILE\n");
        return;
    }
    size_t len;
    const char *data = vfs_read_ptr(args, &len);
    if (!data) {
        vga_puts("file not found\n");
        return;
    }
    vga_puts(data);
    if (len == 0 || data[len - 1] != '\n') {
        vga_putc('\n');
    }
}

static void cmd_touch(char *args) {
    if (*args == '\0') {
        vga_puts("usage: touch FILE\n");
        return;
    }
    int r = vfs_touch(args);
    if (r == 0) {
        vga_puts("ok\n");
    } else {
        vga_puts("touch failed\n");
    }
}

static void cmd_rm(char *args) {
    if (*args == '\0') {
        vga_puts("usage: rm FILE\n");
        return;
    }
    if (vfs_remove(args) == 0) {
        vga_puts("ok\n");
    } else {
        vga_puts("file not found\n");
    }
}

static void cmd_write_common(char *args, int append) {
    char *name;
    char *text;

    if (*args == '\0') {
        vga_puts(append ? "usage: append FILE TEXT\n" : "usage: write FILE TEXT\n");
        return;
    }

    split_first_arg(args, &name, &text);
    if (*name == '\0' || *text == '\0') {
        vga_puts(append ? "usage: append FILE TEXT\n" : "usage: write FILE TEXT\n");
        return;
    }

    int r = append ? vfs_append(name, text) : vfs_write(name, text);
    if (r == 0) {
        vga_puts("ok\n");
    } else {
        vga_puts("write failed\n");
    }
}

static void cmd_edit(char *args) {
    if (*args == '\0') {
        vga_puts("usage: edit FILE\n");
        return;
    }
    if (vfs_touch(args) != 0) {
        vga_puts("cannot open file\n");
        return;
    }
    editor_edit_file(args);
    vga_clear();
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

/* Forward declarations for script and CosyPy - will be implemented in separate files */
extern void cospy_repl(void);
extern int cospy_run_file(const char *filename);
extern int script_run(const char *filename);

static void cmd_python(void) {
    cospy_repl();
}

static void cmd_run(char *args) {
    if (*args == '\0') {
        vga_puts("usage: run FILE.sh\n");
        return;
    }
    if (script_run(args) != 0) {
        vga_puts("script error\n");
    }
}

static void cmd_pyrun(char *args) {
    if (*args == '\0') {
        vga_puts("usage: pyrun FILE.py\n");
        return;
    }
    if (cospy_run_file(args) != 0) {
        vga_puts("script error\n");
    }
}

void shell_run(void) {
    char line[SHELL_LINE_MAX];

    vga_puts("Type 'help' for commands.\n");
    for (;;) {
        /* Show current directory in prompt */
        vga_puts(vfs_getcwd());
        vga_puts("> ");
        keyboard_readline(line, sizeof(line));

        if (line[0] == '\0') {
            continue;
        }

        if (strcmp(line, "!!") == 0) {
            if (history_count == 0) {
                vga_puts("no history\n");
                continue;
            }
            strcpy(line, history[history_count - 1]);
            vga_puts(line);
            vga_putc('\n');
        }

        save_history(line);

        char *cmd;
        char *args;
        if (!split_cmd(line, &cmd, &args)) {
            continue;
        }

        if (strcmp(cmd, "help") == 0) {
            cmd_help();
        } else if (strcmp(cmd, "clear") == 0) {
            vga_clear();
        } else if (strcmp(cmd, "echo") == 0) {
            vga_puts(args);
            vga_putc('\n');
        } else if (strcmp(cmd, "mem") == 0) {
            cmd_mem();
        } else if (strcmp(cmd, "history") == 0) {
            cmd_history();
        } else if (strcmp(cmd, "lang") == 0) {
            lang_repl();
        } else if (strcmp(cmd, "python") == 0) {
            cmd_python();
        } else if (strcmp(cmd, "ls") == 0) {
            cmd_ls();
        } else if (strcmp(cmd, "cat") == 0) {
            cmd_cat(args);
        } else if (strcmp(cmd, "touch") == 0) {
            cmd_touch(args);
        } else if (strcmp(cmd, "rm") == 0) {
            cmd_rm(args);
        } else if (strcmp(cmd, "write") == 0) {
            cmd_write_common(args, 0);
        } else if (strcmp(cmd, "append") == 0) {
            cmd_write_common(args, 1);
        } else if (strcmp(cmd, "edit") == 0) {
            cmd_edit(args);
        } else if (strcmp(cmd, "pwd") == 0) {
            cmd_pwd();
        } else if (strcmp(cmd, "cd") == 0) {
            cmd_cd(args);
        } else if (strcmp(cmd, "mkdir") == 0) {
            cmd_mkdir(args);
        } else if (strcmp(cmd, "rmdir") == 0) {
            cmd_rmdir(args);
        } else if (strcmp(cmd, "run") == 0) {
            cmd_run(args);
        } else if (strcmp(cmd, "pyrun") == 0) {
            cmd_pyrun(args);
        } else if (strcmp(cmd, "reboot") == 0) {
            cmd_reboot();
        } else {
            vga_puts("Unknown command. Try 'help'.\n");
        }
    }
}
