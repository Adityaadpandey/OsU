#ifndef KSTRING_H
#define KSTRING_H

#include <stddef.h>

size_t strlen(const char *s);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, size_t n);
char *strcpy(char *dst, const char *src);
void *memset(void *dst, int c, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
int atoi(const char *s);

#endif
