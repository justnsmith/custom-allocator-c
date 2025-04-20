#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>
#include <stdbool.h>

#define HEAP_CAPACITY 640000
#define ALIGNMENT 16

typedef struct BlockHeader {
    size_t size;
    bool free;
    struct BlockHeader* next;
} BlockHeader;

size_t align(size_t alloc_size);
void* heap_alloc(size_t requested_bytes);
void heap_free(void* block_ptr);
void* heap_realloc(void* block_ptr, size_t new_size);
void* split_block(BlockHeader* block_ptr, size_t total_size);
void print_heap();

#endif // ALLOCATOR_H
