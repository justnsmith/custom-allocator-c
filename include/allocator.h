#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>
#include <stdbool.h>

#define HEAP_CAPACITY 640000
#define ALIGNMENT 16

typedef struct {
    size_t size;
    bool free;
    struct BlockHeader* next;
} BlockHeader;

void* heap_alloc(size_t size);
size_t align(size_t size);

#endif // ALLOCATOR_H
