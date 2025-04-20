#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include "allocator.h"

char heap[HEAP_CAPACITY] = { 0 };
size_t heap_size = 0;

BlockHeader* first_block = NULL;

size_t align(size_t alloc_size) {
    if (alloc_size % ALIGNMENT != 0) {
        alloc_size += ALIGNMENT - (alloc_size % ALIGNMENT);
    }
    return alloc_size;
}

void* heap_alloc(size_t requested_bytes) {
    size_t total_size = requested_bytes + sizeof(BlockHeader);
    total_size = align(total_size);

    assert(heap_size + total_size <= HEAP_CAPACITY);

    void* result = heap + heap_size;

    BlockHeader* curr = first_block;

    if (curr == NULL) {
        curr = (BlockHeader*) result;
        curr->size = total_size;
        curr->free = false;
        curr->next = NULL;
        first_block = curr;
    }
    else {
        while (curr->next != NULL) {
            curr = curr->next;
        }
        BlockHeader* new_block = (BlockHeader*) result;
        new_block->size = total_size;
        new_block->free = false;
        new_block->next = NULL;
        curr->next = new_block;
        curr = new_block;
    }
    heap_size += total_size;

    printf("Allocated %zu bytes at %p\n", total_size, result);
    assert(((uintptr_t)result % ALIGNMENT) == 0);

    return (void *)(curr + 1);
}

void heap_free(void* block_ptr) {
    if (block_ptr == NULL) return;

    BlockHeader* header = (BlockHeader*)block_ptr - 1;
    printf("%p\n", header);
    header->free = true;

    // Forward Coalescing
    if (header->next != NULL && header->next->free == true) {
        BlockHeader* next = header->next;
        header->size += next->size;
        header->next = next->next;
        printf("(FORWARD): Coalesced into size %zu\n", header->size);
    }

    // Backward Coalescing
    BlockHeader* curr = first_block;
    BlockHeader* prev = NULL;

    while (curr != NULL && curr != header) {
        prev = curr;
        curr = curr->next;
    }

    if (prev != NULL && prev->free == true) {
        prev->size += header->size;
        prev->next = header->next;
        printf("(BACKWARD): Coalesced into size %zu\n", prev->size);
    }
}

void print_heap() {
    BlockHeader* curr = first_block;
    int i = 0;
    while (curr) {
        printf("Block %d — Size: %zu | Free: %d | Addr: %p\n", i++, curr->size, curr->free, (void*)curr);
        curr = curr->next;
    }
}
