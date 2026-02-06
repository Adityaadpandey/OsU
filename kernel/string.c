#include "string.h"

size_t strlen(const char *s) {
    size_t n = 0;
    while (s[n]) {
        n++;
    }
    return n;
}

int strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i] || a[i] == '\0' || b[i] == '\0') {
            return (unsigned char)a[i] - (unsigned char)b[i];
        }
    }
    return 0;
}

char *strcpy(char *dst, const char *src) {
    size_t i = 0;
    while (src[i]) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
    return dst;
}

void *memset(void *dst, int c, size_t n) {
    unsigned char *p = (unsigned char *)dst;
    for (size_t i = 0; i < n; i++) {
        p[i] = (unsigned char)c;
    }
    return dst;
}

void *memcpy(void *dst, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dst;
}

int atoi(const char *s) {
    int sign = 1;
    int value = 0;

    while (*s == ' ' || *s == '\t') {
        s++;
    }

    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+') {
        s++;
    }

    while (*s >= '0' && *s <= '9') {
        value = value * 10 + (*s - '0');
        s++;
    }

    return value * sign;
}
