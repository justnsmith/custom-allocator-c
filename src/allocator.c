#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include "allocator.h"

char heap[HEAP_CAPACITY] = { 0 };
size_t heap_size = 0;

size_t align(size_t alloc_size) {
    if (alloc_size % ALIGNMENT != 0) {
        alloc_size += ALIGNMENT - (alloc_size % ALIGNMENT);
    }
    return alloc_size;
}

void* heap_alloc(size_t requested_bytes) {
    size_t total_size = requested_bytes + sizeof(BlockHeader);
    requested_bytes = align(total_size);
    heap_size = align(heap_size);

    assert(heap_size + total_size <= HEAP_CAPACITY);

    BlockHeader* curr = (BlockHeader*) heap;
    BlockHeader* prev = NULL;

    while((char *) curr < heap + heap_size && curr->next != NULL) {
        if (curr->free == true && curr->size >= requested_bytes) {
            curr->free = false;
            return (void *) (curr + 1);
        }
        prev = curr;
        curr = curr->next;
    }

    void* result = heap + heap_size;
    BlockHeader* header = (BlockHeader*) result;
    heap_size += total_size;

    header->size = requested_bytes;
    header->free = false;
    header->next = NULL;

    if (prev != NULL) {
        prev->next = header;
    }

    printf("Allocated %zu bytes at %p\n", requested_bytes, result);
    assert(((uintptr_t)result % ALIGNMENT) == 0);

    return (void *)(header + 1);
}

void heap_free(void* block_ptr) {
    if (block_ptr == NULL) {
        return;
    }

    if ((char *)block_ptr < heap || (char *)block_ptr > heap + heap_size) {
        printf("Pointer is out of bounds.\n");
        return;
    }

    BlockHeader* header = (BlockHeader*) block_ptr - 1;
    header->free = true;
}
