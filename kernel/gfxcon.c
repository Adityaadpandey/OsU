/*
 * gfxcon.c - Graphics Console Implementation
 * Text rendering on VESA framebuffer
 */

#include "gfxcon.h"
#include "vesa.h"

/* Console state */
static int gfx_active = 0;
static int cursor_row = 0;
static int cursor_col = 0;
static uint32_t fg_color = 0x00FF00;  /* Green */
static uint32_t bg_color = 0x000000;  /* Black */
static int gfx_cols = 0;
static int gfx_rows = 0;

void gfxcon_init(void) {
    if (!vesa_enabled) {
        gfx_active = 0;
        return;
    }

    gfx_active = 1;
    cursor_row = 0;
    cursor_col = 0;
    fg_color = 0x00FF00;  /* Green like VGA */
    bg_color = 0x000000;  /* Black */
    gfx_cols = (int)(vesa_width / 8);
    gfx_rows = (int)(vesa_height / 16);
    if (gfx_cols <= 0) gfx_cols = 100;
    if (gfx_rows <= 0) gfx_rows = 37;

    /* Clear the screen */
    gfxcon_clear();
}

int gfxcon_active(void) {
    return gfx_active;
}

void gfxcon_clear(void) {
    if (!gfx_active) return;

    vesa_clear(bg_color);
    cursor_row = 0;
    cursor_col = 0;
}

static void scroll_up(void) {
    if (!gfx_active) return;

    int bpp = vesa_bpp / 8;
    if (bpp == 4) {
        uint32_t *fb = (uint32_t *)vesa_framebuffer;
        uint32_t row_pixels = vesa_pitch / 4;

        /* Copy each row up by 16 pixels (one character height) */
        for (int y = 0; y < (int)vesa_height - 16; y++) {
            for (int x = 0; x < (int)vesa_width; x++) {
                fb[y * row_pixels + x] = fb[(y + 16) * row_pixels + x];
            }
        }

        /* Clear the last row */
        for (int y = (int)vesa_height - 16; y < (int)vesa_height; y++) {
            for (int x = 0; x < (int)vesa_width; x++) {
                fb[y * row_pixels + x] = bg_color;
            }
        }
        return;
    }

    if (bpp == 3) {
        uint8_t *fb = (uint8_t *)vesa_framebuffer;
        for (int y = 0; y < (int)vesa_height - 16; y++) {
            uint32_t dst = (uint32_t)y * vesa_pitch;
            uint32_t src = (uint32_t)(y + 16) * vesa_pitch;
            for (int x = 0; x < (int)vesa_width * 3; x++) {
                fb[dst + x] = fb[src + x];
            }
        }

        for (int y = (int)vesa_height - 16; y < (int)vesa_height; y++) {
            uint32_t row = (uint32_t)y * vesa_pitch;
            for (int x = 0; x < (int)vesa_width; x++) {
                uint32_t off = row + x * 3;
                fb[off + 0] = (uint8_t)(bg_color & 0xFF);
                fb[off + 1] = (uint8_t)((bg_color >> 8) & 0xFF);
                fb[off + 2] = (uint8_t)((bg_color >> 16) & 0xFF);
            }
        }
    }
}

void gfxcon_putc(char c) {
    if (!gfx_active) return;

    if (c == '\n') {
        cursor_col = 0;
        cursor_row++;
    } else if (c == '\r') {
        cursor_col = 0;
    } else if (c == '\t') {
        cursor_col = (cursor_col + 8) & ~7;
    } else if (c == '\b') {
        if (cursor_col > 0) {
            cursor_col--;
            /* Clear the character */
            int x = cursor_col * 8;
            int y = cursor_row * 16;
            vesa_put_char(x, y, ' ', fg_color, bg_color);
        }
    } else if (c >= 32) {
        int x = cursor_col * 8;
        int y = cursor_row * 16;
        vesa_put_char(x, y, c, fg_color, bg_color);
        cursor_col++;
    }

    /* Handle line wrap */
    if (cursor_col >= gfx_cols) {
        cursor_col = 0;
        cursor_row++;
    }

    /* Handle scroll */
    while (cursor_row >= gfx_rows) {
        scroll_up();
        cursor_row--;
    }
}

void gfxcon_puts(const char *s) {
    while (*s) {
        gfxcon_putc(*s++);
    }
}

void gfxcon_set_fg(uint32_t color) {
    fg_color = color;
}

void gfxcon_set_bg(uint32_t color) {
    bg_color = color;
}

int gfxcon_get_row(void) {
    return cursor_row;
}

int gfxcon_get_col(void) {
    return cursor_col;
}

void gfxcon_set_cursor(int row, int col) {
    cursor_row = row;
    cursor_col = col;

    if (cursor_row < 0) cursor_row = 0;
    if (cursor_row >= gfx_rows) cursor_row = gfx_rows - 1;
    if (cursor_col < 0) cursor_col = 0;
    if (cursor_col >= gfx_cols) cursor_col = gfx_cols - 1;
}

int gfxcon_cols(void) {
    return gfx_cols;
}

int gfxcon_rows(void) {
    return gfx_rows;
}
