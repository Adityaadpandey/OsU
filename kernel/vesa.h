/*
 * vesa.h - VESA VBE Graphics Driver
 */

#ifndef VESA_H
#define VESA_H

#include <stdint.h>

/* VBE mode info structure (partial, key fields) */
typedef struct __attribute__((packed)) {
    uint16_t attributes;
    uint8_t  window_a;
    uint8_t  window_b;
    uint16_t granularity;
    uint16_t window_size;
    uint16_t segment_a;
    uint16_t segment_b;
    uint32_t win_func_ptr;
    uint16_t pitch;           /* Bytes per scanline */
    uint16_t width;           /* Screen width in pixels */
    uint16_t height;          /* Screen height in pixels */
    uint8_t  w_char;
    uint8_t  y_char;
    uint8_t  planes;
    uint8_t  bpp;             /* Bits per pixel */
    uint8_t  banks;
    uint8_t  memory_model;
    uint8_t  bank_size;
    uint8_t  image_pages;
    uint8_t  reserved0;
    uint8_t  red_mask;
    uint8_t  red_position;
    uint8_t  green_mask;
    uint8_t  green_position;
    uint8_t  blue_mask;
    uint8_t  blue_position;
    uint8_t  reserved_mask;
    uint8_t  reserved_position;
    uint8_t  direct_color_attributes;
    uint32_t framebuffer;     /* Physical address of framebuffer */
    uint32_t off_screen_mem_off;
    uint16_t off_screen_mem_size;
    uint8_t  reserved1[206];
} vbe_mode_info_t;

/* Graphics mode info */
extern uint32_t vesa_framebuffer;
extern uint16_t vesa_width;
extern uint16_t vesa_height;
extern uint16_t vesa_pitch;
extern uint8_t  vesa_bpp;
extern int      vesa_enabled;

/* Initialize VESA from bootloader-provided info */
void vesa_init(void);

/* Basic drawing functions */
void vesa_put_pixel(int x, int y, uint32_t color);
uint32_t vesa_get_pixel(int x, int y);
void vesa_fill_rect(int x, int y, int w, int h, uint32_t color);
void vesa_clear(uint32_t color);
void vesa_set_backbuffer(int enable);
void vesa_present(void);

/* Text rendering (8x16 bitmap font) */
void vesa_put_char(int x, int y, char c, uint32_t fg, uint32_t bg);
void vesa_put_string(int x, int y, const char *s, uint32_t fg, uint32_t bg);

/* Color helpers */
#define VESA_RGB(r, g, b) (((r) << 16) | ((g) << 8) | (b))
#define VESA_BLACK   0x000000
#define VESA_WHITE   0xFFFFFF
#define VESA_RED     0xFF0000
#define VESA_GREEN   0x00FF00
#define VESA_BLUE    0x0000FF
#define VESA_YELLOW  0xFFFF00
#define VESA_CYAN    0x00FFFF
#define VESA_MAGENTA 0xFF00FF

/* Draw line */
void vesa_draw_line(int x0, int y0, int x1, int y1, uint32_t color);

/* Draw rectangle outline */
void vesa_draw_rect(int x, int y, int w, int h, uint32_t color);

#endif /* VESA_H */
