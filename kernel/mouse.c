#include "mouse.h"

#include "idt.h"
#include "io.h"

#define PS2_DATA_PORT 0x60
#define PS2_STATUS_PORT 0x64
#define PS2_CMD_PORT 0x64

static volatile int mouse_x = 0;
static volatile int mouse_y = 0;
static volatile uint8_t mouse_buttons = 0;
static volatile int8_t mouse_dx = 0;
static volatile int8_t mouse_dy = 0;
static volatile int mouse_updated = 0;

static int screen_w = 800;
static int screen_h = 600;

static uint8_t packet[3];
static int packet_index = 0;

static void ps2_wait_read(void) {
    while ((inb(PS2_STATUS_PORT) & 0x01) == 0) {
        /* wait */
    }
}

static void ps2_wait_write(void) {
    while (inb(PS2_STATUS_PORT) & 0x02) {
        /* wait */
    }
}

static void mouse_write(uint8_t value) {
    ps2_wait_write();
    outb(PS2_CMD_PORT, 0xD4);
    ps2_wait_write();
    outb(PS2_DATA_PORT, value);
}

static uint8_t mouse_read(void) {
    ps2_wait_read();
    return inb(PS2_DATA_PORT);
}

static void mouse_irq_handler(registers_t *r) {
    (void)r;

    uint8_t status = inb(PS2_STATUS_PORT);
    if ((status & 0x20) == 0) {
        return;
    }

    uint8_t data = inb(PS2_DATA_PORT);

    if (packet_index == 0 && (data & 0x08) == 0) {
        return;
    }

    packet[packet_index++] = data;
    if (packet_index < 3) {
        return;
    }
    packet_index = 0;

    uint8_t b = packet[0];
    int8_t dx = (int8_t)packet[1];
    int8_t dy = (int8_t)packet[2];

    int x = mouse_x + dx;
    int y = mouse_y - dy;

    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= screen_w) x = screen_w - 1;
    if (y >= screen_h) y = screen_h - 1;

    mouse_x = x;
    mouse_y = y;
    mouse_buttons = (uint8_t)(b & 0x07);
    mouse_dx = dx;
    mouse_dy = dy;
    mouse_updated = 1;
}

void mouse_set_bounds(int width, int height) {
    if (width > 0) screen_w = width;
    if (height > 0) screen_h = height;

    if (mouse_x >= screen_w) mouse_x = screen_w - 1;
    if (mouse_y >= screen_h) mouse_y = screen_h - 1;
}

void mouse_init(void) {
    packet_index = 0;
    mouse_updated = 0;
    mouse_x = screen_w / 2;
    mouse_y = screen_h / 2;
    mouse_buttons = 0;

    /* Enable keyboard and auxiliary devices */
    ps2_wait_write();
    outb(PS2_CMD_PORT, 0xAE); /* Enable keyboard interface */
    ps2_wait_write();
    outb(PS2_CMD_PORT, 0xA8); /* Enable auxiliary device */

    ps2_wait_write();
    outb(PS2_CMD_PORT, 0x20); /* Read controller command byte */
    ps2_wait_read();
    uint8_t status = inb(PS2_DATA_PORT);
    status &= (uint8_t)~0x10; /* Clear keyboard disable */
    status &= (uint8_t)~0x20; /* Clear mouse disable */
    status |= 0x02;           /* Enable IRQ12 */
    status |= 0x01;           /* Ensure IRQ1 stays enabled */
    ps2_wait_write();
    outb(PS2_CMD_PORT, 0x60); /* Write controller command byte */
    ps2_wait_write();
    outb(PS2_DATA_PORT, status);

    mouse_write(0xF6); /* Set defaults */
    mouse_read();
    mouse_write(0xF4); /* Enable data reporting */
    mouse_read();

    idt_register_handler(44, mouse_irq_handler);
}

int mouse_poll(mouse_state_t *out) {
    if (!out) {
        return 0;
    }
    if (!mouse_updated) {
        return 0;
    }
    out->x = mouse_x;
    out->y = mouse_y;
    out->buttons = mouse_buttons;
    out->dx = mouse_dx;
    out->dy = mouse_dy;
    mouse_updated = 0;
    return 1;
}
