#include "editor.h"

#include <stddef.h>
#include <stdint.h>

#include "fs.h"
#include "keyboard.h"
#include "string.h"
#include "vfs.h"
#include "vga.h"

typedef enum {
    MODE_NORMAL = 0,
    MODE_INSERT = 1,
} mode_t;

static size_t line_start(const char *buf, size_t pos) {
    while (pos > 0 && buf[pos - 1] != '\n') {
        pos--;
    }
    return pos;
}

static size_t line_end(const char *buf, size_t len, size_t pos) {
    while (pos < len && buf[pos] != '\n') {
        pos++;
    }
    return pos;
}

static void draw_editor(const char *name, const char *buf, size_t len, size_t cursor, mode_t mode, const char *msg) {
    vga_clear();
    vga_printf("[%s] %s | %s\n", mode == MODE_INSERT ? "INSERT" : "NORMAL", name, msg ? msg : "");

    size_t i = 0;
    size_t row = 1;
    size_t col = 0;
    size_t cur_row = 1;
    size_t cur_col = 0;

    while (i < len && row < VGA_HEIGHT) {
        if (i == cursor) {
            cur_row = row;
            cur_col = col;
        }

        char c = buf[i++];
        if (c == '\n') {
            vga_putc('\n');
            row++;
            col = 0;
            continue;
        }

        if (col >= VGA_WIDTH) {
            vga_putc('\n');
            row++;
            col = 0;
            if (row >= VGA_HEIGHT) {
                break;
            }
        }
        vga_putc(c);
        col++;
    }

    if (cursor == len && row < VGA_HEIGHT) {
        cur_row = row;
        cur_col = col;
    }

    if (cur_row >= VGA_HEIGHT) {
        cur_row = VGA_HEIGHT - 1;
    }
    if (cur_col >= VGA_WIDTH) {
        cur_col = VGA_WIDTH - 1;
    }

    vga_set_cursor((uint8_t)cur_col, (uint8_t)cur_row);
}

static int read_colon_cmd(char *cmd, size_t cap) {
    size_t i = 0;
    vga_set_cursor(0, VGA_HEIGHT - 1);
    vga_puts(":");

    while (1) {
        char c = keyboard_getchar();

        /* Skip null characters */
        if (c == 0) {
            continue;
        }

        /* Handle Enter key - both '\n' and '\r' should work */
        if (c == '\n' || c == '\r') {
            cmd[i] = '\0';
            while (i > 0 && cmd[i - 1] == ' ') {
                cmd[--i] = '\0';
            }
            return 0;
        }
        /* Handle Escape - cancel command */
        if (c == 27) {
            cmd[0] = '\0';
            return -1;
        }
        if (c == '\b') {
            if (i > 0) {
                i--;
                vga_putc('\b');
            }
            continue;
        }
        if (i + 1 < cap && c >= 32 && c <= 126) {
            cmd[i++] = c;
            vga_putc(c);
        }
    }
}

static int insert_char(char *buf, size_t *len, size_t *cursor, char c) {
    if (*len + 1 >= FS_MAX_FILE_SIZE) {
        return -1;
    }
    for (size_t i = *len; i > *cursor; i--) {
        buf[i] = buf[i - 1];
    }
    buf[*cursor] = c;
    (*len)++;
    (*cursor)++;
    buf[*len] = '\0';
    return 0;
}

static void delete_at(char *buf, size_t *len, size_t pos) {
    if (pos >= *len) {
        return;
    }
    for (size_t i = pos; i + 1 < *len; i++) {
        buf[i] = buf[i + 1];
    }
    (*len)--;
    buf[*len] = '\0';
}

int editor_edit_file(const char *name) {
    char text[FS_MAX_FILE_SIZE];
    char colon[16];
    const char *msg = "i insert | h/j/k/l move | :w :q :wq | Ctrl+S save";

    size_t len = 0;
    const char *src = vfs_read_ptr(name, &len);
    if (!src) {
        text[0] = '\0';
        len = 0;
    } else {
        memcpy(text, src, len);
        text[len] = '\0';
    }

    size_t cursor = 0;
    mode_t mode = MODE_NORMAL;
    int dirty = 0;

    while (1) {
        draw_editor(name, text, len, cursor, mode, msg);
        msg = "";

        char c = keyboard_getchar();
        if (mode == MODE_INSERT) {
            if ((unsigned char)c == 19) { /* Ctrl+S */
                if (vfs_write_raw(name, text, len) == 0) {
                    dirty = 0;
                    msg = "written";
                } else {
                    msg = "write failed";
                }
                continue;
            }
            if (c == 27) {
                mode = MODE_NORMAL;
                continue;
            }
            if (c == '\b') {
                if (cursor > 0) {
                    cursor--;
                    delete_at(text, &len, cursor);
                    dirty = 1;
                }
                continue;
            }
            if (c == '\n' || c == '\r') {
                if (insert_char(text, &len, &cursor, '\n') == 0) {
                    dirty = 1;
                } else {
                    msg = "file full";
                }
                continue;
            }
            if (c >= 32 && c <= 126) {
                if (insert_char(text, &len, &cursor, c) == 0) {
                    dirty = 1;
                } else {
                    msg = "file full";
                }
            }
            continue;
        }

        if ((unsigned char)c == 19) { /* Ctrl+S */
            if (vfs_write_raw(name, text, len) == 0) {
                dirty = 0;
                msg = "written";
            } else {
                msg = "write failed";
            }
        } else if (c == 'i') {
            mode = MODE_INSERT;
        } else if (c == 'h') {
            if (cursor > 0) {
                cursor--;
            }
        } else if (c == 'l') {
            if (cursor < len) {
                cursor++;
            }
        } else if (c == 'x') {
            if (cursor < len) {
                delete_at(text, &len, cursor);
                dirty = 1;
            }
        } else if (c == 'j') {
            size_t col = cursor - line_start(text, cursor);
            size_t next_start = line_end(text, len, cursor);
            if (next_start < len && text[next_start] == '\n') {
                next_start++;
                size_t next_end = line_end(text, len, next_start);
                size_t next_len = next_end - next_start;
                cursor = next_start + (col < next_len ? col : next_len);
            }
        } else if (c == 'k') {
            size_t cur_start = line_start(text, cursor);
            if (cur_start > 0) {
                size_t col = cursor - cur_start;
                size_t prev_end = cur_start - 1;
                size_t prev_start = line_start(text, prev_end);
                size_t prev_len = prev_end - prev_start;
                cursor = prev_start + (col < prev_len ? col : prev_len);
            }
        } else if (c == ':') {
            if (read_colon_cmd(colon, sizeof(colon)) < 0 || colon[0] == '\0') {
                /* User cancelled with Escape or empty command */
                continue;
            }
            if (strcmp(colon, "w") == 0) {
                if (vfs_write_raw(name, text, len) == 0) {
                    dirty = 0;
                    msg = "written";
                } else {
                    msg = "write failed";
                }
            } else if (strcmp(colon, "q") == 0) {
                if (dirty) {
                    msg = "unsaved changes";
                } else {
                    return 0;
                }
            } else if (strcmp(colon, "q!") == 0) {
                return 0;
            } else if (strcmp(colon, "wq") == 0 || strcmp(colon, "x") == 0) {
                if (vfs_write_raw(name, text, len) == 0) {
                    return 0;
                }
                msg = "write failed";
            } else {
                msg = "unknown :cmd";
            }
        }
    }
}
