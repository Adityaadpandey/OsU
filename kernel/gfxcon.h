/*
 * gfxcon.h - Graphics Console
 * Text rendering on VESA framebuffer with VGA-like API
 */

#ifndef GFXCON_H
#define GFXCON_H

#include <stdint.h>
#include <stddef.h>

/* Initialize graphics console (call after vesa_init) */
void gfxcon_init(void);

/* Check if graphics console is active */
int gfxcon_active(void);

/* Clear the screen */
void gfxcon_clear(void);

/* Put a character at current cursor position */
void gfxcon_putc(char c);

/* Put a string */
void gfxcon_puts(const char *s);

/* Set foreground and background colors (RGB) */
void gfxcon_set_fg(uint32_t color);
void gfxcon_set_bg(uint32_t color);

/* Get cursor position */
int gfxcon_get_row(void);
int gfxcon_get_col(void);

/* Set cursor position */
void gfxcon_set_cursor(int row, int col);

/* Console dimensions in characters */
int gfxcon_cols(void);
int gfxcon_rows(void);

#endif /* GFXCON_H */
