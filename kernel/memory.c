#include "memory.h"

extern uint8_t _kernel_end;

static uintptr_t heap_start;
static uintptr_t heap_end;
static uintptr_t heap_curr;

static uintptr_t align16(uintptr_t v) {
    return (v + 15u) & ~((uintptr_t)15u);
}

void memory_init(void) {
    heap_start = align16((uintptr_t)&_kernel_end);
    heap_end = 0x00400000;
    heap_curr = heap_start;
}

void *kmalloc(size_t size) {
    if (size == 0) {
        return 0;
    }

    uintptr_t begin = align16(heap_curr);
    uintptr_t end = align16(begin + size);
    if (end > heap_end) {
        return 0;
    }

    heap_curr = end;
    return (void *)begin;
}

uintptr_t memory_heap_start(void) {
    return heap_start;
}

uintptr_t memory_heap_end(void) {
    return heap_end;
}

uintptr_t memory_heap_used(void) {
    return heap_curr - heap_start;
}
