#include "gui.h"

#include <stddef.h>
#include <stdint.h>

#include "keyboard.h"
#include "mouse.h"
#include "timer.h"
#include "vesa.h"

#define GUI_BAR_H 24
#define WIN_TITLE_H 18
#define WIN_PAD 6

typedef struct {
    int x;
    int y;
    int w;
    int h;
    int dragging;
    int drag_off_x;
    int drag_off_y;
} gui_window_t;

static gui_window_t main_win;

static char input_line[128];
static size_t input_len = 0;

static char log_lines[8][64];
static size_t log_count = 0;
static char last_key[16];

static void draw_rect(int x, int y, int w, int h, uint32_t color) {
    vesa_fill_rect(x, y, w, h, color);
}

static void draw_string(int x, int y, const char *s, uint32_t fg, uint32_t bg) {
    vesa_put_string(x, y, s, fg, bg);
}

static void draw_background(void) {
    if (!vesa_enabled) return;

    for (int y = 0; y < (int)vesa_height; y++) {
        uint8_t shade = (uint8_t)(40 + (y * 80) / vesa_height);
        uint32_t color = VESA_RGB(shade / 2, shade / 2, shade);
        vesa_draw_line(0, y, vesa_width - 1, y, color);
    }
}

static void draw_taskbar(void) {
    int y = vesa_height - GUI_BAR_H;
    draw_rect(0, y, vesa_width, GUI_BAR_H, VESA_RGB(20, 20, 30));
    vesa_draw_line(0, y, vesa_width - 1, y, VESA_RGB(80, 80, 120));
    draw_string(8, y + 4, "OsU", VESA_WHITE, VESA_RGB(20, 20, 30));

    uint32_t secs = timer_get_ticks() / TIMER_FREQ;
    char buf[32];
    int n = 0;
    if (secs == 0) {
        buf[n++] = '0';
    } else {
        char tmp[16];
        int t = 0;
        while (secs > 0 && t < 16) {
            tmp[t++] = (char)('0' + (secs % 10));
            secs /= 10;
        }
        while (t--) buf[n++] = tmp[t];
    }
    buf[n++] = 's';
    buf[n] = '\0';

    int tx = vesa_width - (n * 8) - 8;
    if (tx < 0) tx = 0;
    draw_string(tx, y + 4, buf, VESA_WHITE, VESA_RGB(20, 20, 30));
}

static void draw_window(const gui_window_t *win) {
    int x = win->x;
    int y = win->y;
    int w = win->w;
    int h = win->h;

    draw_rect(x, y, w, h, VESA_RGB(30, 30, 40));
    vesa_draw_rect(x, y, w, h, VESA_RGB(120, 120, 160));
    draw_rect(x, y, w, WIN_TITLE_H, VESA_RGB(50, 50, 80));
    vesa_draw_line(x, y + WIN_TITLE_H - 1, x + w - 1, y + WIN_TITLE_H - 1, VESA_RGB(90, 90, 120));
    draw_string(x + WIN_PAD, y + 2, "OsU Terminal", VESA_WHITE, VESA_RGB(50, 50, 80));

    int content_x = x + WIN_PAD;
    int content_y = y + WIN_TITLE_H + WIN_PAD;
    int max_lines = 6;

    for (int i = 0; i < (int)log_count && i < max_lines; i++) {
        draw_string(content_x, content_y + i * 16, log_lines[i], VESA_RGB(210, 230, 255), VESA_RGB(30, 30, 40));
    }

    char prompt[160];
    size_t p = 0;
    prompt[p++] = '>';
    prompt[p++] = ' ';
    for (size_t i = 0; i < input_len && p + 1 < sizeof(prompt); i++) {
        prompt[p++] = input_line[i];
    }
    prompt[p] = '\0';

    draw_string(content_x, content_y + max_lines * 16, prompt, VESA_GREEN, VESA_RGB(30, 30, 40));
}

static void draw_cursor(int x, int y) {
    draw_rect(x, y, 8, 8, VESA_WHITE);
    vesa_draw_rect(x, y, 8, 8, VESA_BLACK);
    vesa_put_pixel(x + 3, y + 3, VESA_BLACK);
}

static void gui_log_line(const char *s) {
    if (!s) return;
    if (log_count < 8) {
        size_t i = 0;
        while (s[i] && i + 1 < sizeof(log_lines[0])) {
            log_lines[log_count][i] = s[i];
            i++;
        }
        log_lines[log_count][i] = '\0';
        log_count++;
        return;
    }

    for (size_t i = 1; i < 8; i++) {
        for (size_t j = 0; j + 1 < sizeof(log_lines[0]); j++) {
            log_lines[i - 1][j] = log_lines[i][j];
            if (log_lines[i][j] == '\0') break;
        }
    }
    size_t k = 0;
    while (s[k] && k + 1 < sizeof(log_lines[0])) {
        log_lines[7][k] = s[k];
        k++;
    }
    log_lines[7][k] = '\0';
}

void gui_run(void) {
    if (!vesa_enabled) {
        return;
    }

    vesa_set_backbuffer(1);
    keyboard_flush();
    mouse_set_bounds(vesa_width, vesa_height);
    mouse_init();

    main_win.x = 80;
    main_win.y = 60;
    main_win.w = 520;
    main_win.h = 240;
    main_win.dragging = 0;

    input_len = 0;
    log_count = 0;
    last_key[0] = '\0';
    gui_log_line("GUI ready. Type text and press Enter.");
    gui_log_line("ESC exits to shell.");

    int running = 1;
    mouse_state_t ms = { .x = vesa_width / 2, .y = vesa_height / 2, .buttons = 0, .dx = 0, .dy = 0 };
    uint8_t last_buttons = 0;

    while (running) {
        __asm__ volatile("sti; hlt");
        mouse_state_t tmp;
        if (mouse_poll(&tmp)) {
            ms = tmp;
        }

        if ((ms.buttons & 0x01) && !(last_buttons & 0x01)) {
            int mx = ms.x;
            int my = ms.y;
            if (mx >= main_win.x && mx < main_win.x + main_win.w &&
                my >= main_win.y && my < main_win.y + WIN_TITLE_H) {
                main_win.dragging = 1;
                main_win.drag_off_x = mx - main_win.x;
                main_win.drag_off_y = my - main_win.y;
            }
        } else if (!(ms.buttons & 0x01) && (last_buttons & 0x01)) {
            main_win.dragging = 0;
        }

        if (main_win.dragging) {
            main_win.x = ms.x - main_win.drag_off_x;
            main_win.y = ms.y - main_win.drag_off_y;
            if (main_win.x < 0) main_win.x = 0;
            if (main_win.y < 0) main_win.y = 0;
            if (main_win.x + main_win.w > (int)vesa_width) main_win.x = vesa_width - main_win.w;
            if (main_win.y + main_win.h > (int)vesa_height - GUI_BAR_H) {
                main_win.y = vesa_height - GUI_BAR_H - main_win.h;
            }
        }

        char c;
        while (keyboard_try_getchar(&c)) {
            if (c == 27) {
                running = 0;
                break;
            }
            if (c == '\b') {
                if (input_len > 0) input_len--;
                continue;
            }
            if (c == '\r') continue;
            if (c == '\n') {
                input_line[input_len] = '\0';
                if (input_len > 0) {
                    gui_log_line(input_line);
                }
                input_len = 0;
                continue;
            }
            if (c >= 32 && c <= 126 && input_len + 1 < sizeof(input_line)) {
                input_line[input_len++] = c;
            }

            if (c >= 32 && c <= 126) {
                last_key[0] = c;
                last_key[1] = '\0';
            } else {
                last_key[0] = '^';
                last_key[1] = '@' + (c & 0x1F);
                last_key[2] = '\0';
            }
        }

        draw_background();
        draw_taskbar();
        draw_window(&main_win);
        if (last_key[0]) {
            draw_string(main_win.x + WIN_PAD, main_win.y + main_win.h - 20, last_key, VESA_YELLOW, VESA_RGB(30, 30, 40));
        }
        draw_cursor(ms.x, ms.y);
        vesa_present();

        last_buttons = ms.buttons;
        timer_sleep(16);
    }

    vesa_set_backbuffer(0);
}
