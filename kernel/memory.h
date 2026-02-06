#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>

void memory_init(void);
void *kmalloc(size_t size);

uintptr_t memory_heap_start(void);
uintptr_t memory_heap_end(void);
uintptr_t memory_heap_used(void);

#endif
