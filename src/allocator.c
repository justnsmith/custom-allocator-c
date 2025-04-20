#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "allocator.h"

char heap[HEAP_CAPACITY] = {0};
size_t heap_size = 0;

BlockHeader * first_block = NULL;

size_t align(size_t alloc_size) {
    if (alloc_size % ALIGNMENT != 0) {
        alloc_size += ALIGNMENT - (alloc_size % ALIGNMENT);
    }
    return alloc_size;
}

void * heap_alloc(size_t requested_bytes) {
    size_t total_size = requested_bytes + sizeof(BlockHeader);
    total_size = align(total_size);

    BlockHeader * curr = first_block;

    while (curr != NULL) {
        if (curr -> free == true && curr -> size >= total_size) {
            if (curr -> size >= total_size + sizeof(BlockHeader) + ALIGNMENT) {
               split_block(curr, total_size);
            }
            else {
                curr->free = false;
            }
            printf("Reused block at %p (%zu bytes)\n", curr, curr -> size);
            return (void * )(curr + 1);
        }
        curr = curr -> next;
    }

    assert(heap_size + total_size <= HEAP_CAPACITY);
    void * result = heap + heap_size;

    BlockHeader * new_block = (BlockHeader * ) result;
    new_block -> size = total_size;
    new_block -> free = false;
    new_block -> next = NULL;

    if (first_block == NULL) {
        first_block = new_block;
    } else {
        curr = first_block;
        while (curr -> next != NULL) {
            curr = curr -> next;
        }
        curr -> next = new_block;
    }

    heap_size += total_size;

    printf("Allocated new block of %zu bytes at %p\n", total_size, result);
    assert(((uintptr_t) result % ALIGNMENT) == 0);

    return (void * )(new_block + 1);
}

void heap_free(void * block_ptr) {
    if (block_ptr == NULL) return;

    BlockHeader * header = (BlockHeader * ) block_ptr - 1;
    printf("%p\n", header);
    header -> free = true;

    // Forward Coalescing
    if (header -> next != NULL && header -> next -> free == true) {
        BlockHeader * next = header -> next;
        header -> size += next -> size;
        header -> next = next -> next;
        printf("(FORWARD): Coalesced into size %zu\n", header -> size);
    }

    // Backward Coalescing
    BlockHeader * curr = first_block;
    BlockHeader * prev = NULL;

    while (curr != NULL && curr != header) {
        prev = curr;
        curr = curr -> next;
    }

    if (prev != NULL && prev -> free == true) {
        prev -> size += header -> size;
        prev -> next = header -> next;
        printf("(BACKWARD): Coalesced into size %zu\n", prev -> size);
    }
}

void* heap_realloc(void * ptr, size_t new_size) {
    if (ptr == NULL) {
        return heap_alloc(new_size);
    }

    if (new_size == 0) {
        heap_free(ptr);
        return NULL;
    }

    BlockHeader* curr = (BlockHeader *) ptr - 1;
    size_t total_new_size = align(new_size + sizeof(BlockHeader));

    if (curr->size >= total_new_size) {
        split_block(curr, total_new_size);
        return ptr;
    }

    if (curr->next != NULL && curr->next->free == true && (curr->next->size + curr->size) >= total_new_size) {
        curr->size += curr->next->size + sizeof(BlockHeader);
        curr->next = curr->next->next;
        split_block(curr, total_new_size);
        return ptr;
    }

    void* new_ptr = heap_alloc(new_size);
    if (new_ptr == NULL) return NULL;

    size_t copy_size = curr->size - sizeof(BlockHeader);
    if (new_size < copy_size) {
        copy_size = new_size;
    }

    memcpy(new_ptr, ptr, copy_size);
    heap_free(ptr);
    return new_ptr;
}

void* split_block(BlockHeader* block_ptr, size_t total_size) {
    assert(block_ptr != NULL);
    assert((char*)block_ptr >= (char*)first_block && (char*)block_ptr < heap + heap_size);
    assert(block_ptr->size >= total_size + sizeof(BlockHeader) + ALIGNMENT);

    BlockHeader* secondBox = (BlockHeader *)((char *) block_ptr + total_size);
    secondBox->size = block_ptr->size - total_size;
    secondBox->free = true;
    secondBox->next = block_ptr->next;

    block_ptr->size = total_size;
    block_ptr->next = secondBox;
    block_ptr->free = false;
    return (void *)(block_ptr + 1);
}

void print_heap() {
    BlockHeader * curr = first_block;
    int i = 0;
    while (curr) {
        printf("Block %d — Size: %zu | Free: %d | Addr: %p\n", i++, curr -> size, curr -> free, (void * ) curr);
        curr = curr -> next;
    }
}
