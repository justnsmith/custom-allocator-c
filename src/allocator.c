#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include "allocator.h"

char heap[HEAP_CAPACITY] = { 0 };
size_t heap_size = 0;

size_t align(size_t size) {
    if (size % ALIGNMENT != 0) {
        size += ALIGNMENT - (size % ALIGNMENT);
    }
    return size;
}

void* heap_alloc(size_t size) {
    size = align(size);
    heap_size = align(heap_size);

    assert(heap_size + size <= HEAP_CAPACITY);

    void* result = heap + heap_size;
    heap_size += size;

    printf("Allocated %zu bytes at %p\n", size, result);
    assert(((uintptr_t)result % ALIGNMENT) == 0);

    return result;
}
