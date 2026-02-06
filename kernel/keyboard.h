#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stddef.h>

void keyboard_init(void);
char keyboard_getchar(void);
void keyboard_readline(char *buf, size_t max_len);

#endif
