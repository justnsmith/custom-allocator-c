#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>
#include <stdbool.h>

#define HEAP_CAPACITY 640000
#define ALIGNMENT 16

typedef struct {
    size_t size;
    bool free;
    void* next;
} BlockHeader;

size_t align(size_t alloc_size);
void* heap_alloc(size_t requested_bytes);
void heap_free(void* block_ptr);

#endif // ALLOCATOR_H
