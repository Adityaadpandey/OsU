#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

typedef struct {
    int x;
    int y;
    uint8_t buttons;
    int8_t dx;
    int8_t dy;
} mouse_state_t;

void mouse_init(void);
void mouse_set_bounds(int width, int height);
int mouse_poll(mouse_state_t *out);

#endif
