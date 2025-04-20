#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "allocator.h"

char heap[HEAP_CAPACITY] = {0};                  // heap storage
size_t heap_size = 0;                            // tracks heap size
BlockHeader* first_block = NULL;                 // first heap block
AllocationStrategy current_strategy = FIRST_FIT; // default strategy

size_t align(size_t alloc_size) {
    if (alloc_size % ALIGNMENT != 0) {
        alloc_size += ALIGNMENT - (alloc_size % ALIGNMENT);
    }
    return alloc_size;
}

void* find_fit_first(size_t requested_size) {
    BlockHeader* curr_block = first_block;
    while (curr_block != NULL) {
        if (curr_block->free == true && curr_block->size >= requested_size) {
            return (void *)(curr_block + 1);
        }
        curr_block = curr_block->next;
    }
    return NULL;
}

void* find_fit_best(size_t requested_size) {
    BlockHeader* curr_block = first_block;
    BlockHeader* best_block = NULL;

    while (curr_block != NULL) {
        if (curr_block->free == true && curr_block->size >= requested_size) {
            if (best_block == NULL) {
                best_block = curr_block;
            }
            else if (best_block->size > curr_block->size) {
                best_block = curr_block;
            }
        }
        curr_block = curr_block->next;
    }
    return best_block ? (void*)(best_block + 1) : NULL;
}

void* find_fit_worst(size_t requested_size) {
    BlockHeader* curr_block = first_block;
    BlockHeader* worst_block = NULL;

    while (curr_block != NULL) {
        if (curr_block->free == true && curr_block->size >= requested_size) {
            if (worst_block == NULL) {
                worst_block = curr_block;
            }
            else if (worst_block->size < curr_block->size) {
                worst_block = curr_block;
            }
        }
        curr_block = curr_block->next;
    }
    return worst_block ? (void*)(worst_block + 1) : NULL;
}

void coalesce_blocks(BlockHeader* header) {
    // Forward Coalescing
    while (header->next != NULL && header->next->free == true) {
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

void* heap_alloc(size_t requested_bytes) {
    size_t total_size = align(requested_bytes + sizeof(BlockHeader));
    void* found = NULL;

    switch (current_strategy) {
        case FIRST_FIT:
            found = find_fit_first(total_size);
            break;
        case BEST_FIT:
            found = find_fit_best(total_size);
            break;
        case WORST_FIT:
            found = find_fit_worst(total_size);
            break;
    }

    if (found != NULL) {
        BlockHeader* block = (BlockHeader*) found - 1;

        if (block->size >= total_size + sizeof(BlockHeader) + ALIGNMENT) {
            split_block(block, total_size);
        }
        else {
            block->free = false;
        }
        printf("Reused block at %p (%zu bytes)\n", block, block->size);
        return found;
    }

    // If there are no suitable free blocks, create a new block at the end of heap.
    assert(heap_size + total_size <= HEAP_CAPACITY); // Ensure enough space in heap.

    void* result = heap + heap_size; // Address where the new block will be allocated.

    // Create and initialize the new block
    BlockHeader* new_block = (BlockHeader*) result;
    new_block->size = total_size;
    new_block->free = false;
    new_block->next = NULL;

    // If the heap is empty, set the first block.
    if (first_block == NULL) {
        first_block = new_block; // Set the first block if heap is empty
    } else {
        BlockHeader* curr_block = first_block;
        while (curr_block->next != NULL) {
            curr_block = curr_block->next;
        }
        curr_block->next = new_block;
    }

    // Update the total heap size
    heap_size += total_size;

    printf("Allocated new block of %zu bytes at %p\n", total_size, result);
    assert(((uintptr_t) result % ALIGNMENT) == 0); // Ensure the block is properly aligned

    return (void*)(new_block + 1);
}

void heap_free(void* ptr) {
    assert(ptr != NULL);

    // Check if the pointer is within the valid heap range
    assert(validate_pointer(ptr));

    BlockHeader* header = (BlockHeader*) ptr - 1;
    header->free = true;
    coalesce_blocks(header);
}

void* heap_realloc(void* ptr, size_t new_size) {
    if (ptr == NULL) {
        return heap_alloc(new_size);
    }

    if (new_size == 0) {
        heap_free(ptr);
        return NULL;
    }

     // Check if the pointer is within the valid heap range.
     assert(validate_pointer(ptr));

    BlockHeader* curr = (BlockHeader*) ptr - 1;
    size_t total_new_size = align(new_size + sizeof(BlockHeader));

    // If current block is large enough to fit new size, split the block if possible.
    if (curr->size >= total_new_size) {
        split_block(curr, total_new_size);
        return ptr;
    }

    // If the next block is free and large enough to fit the new size, coalesce the blocks.
    if (curr->next != NULL && curr->next->free == true && (curr->next->size + curr->size) >= total_new_size) {
        curr->size += curr->next->size + sizeof(BlockHeader);
        curr->next = curr->next->next;
        split_block(curr, total_new_size);
        return ptr;
    }

    // If the block cannot be resized in place, allocate a new block and copy data from the old block.
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
    // Safety checs to make sure valid input and that the block can be split
    assert(block_ptr != NULL);
    assert(validate_pointer((void*)block_ptr + 1));
    assert(block_ptr->size >= total_size + sizeof(BlockHeader) + ALIGNMENT);

    // Calculate the address of the new free block after the allocated block
    BlockHeader* secondBox = (BlockHeader*)((char*) block_ptr + total_size);
    secondBox->size = block_ptr->size - total_size; // Remaining size
    secondBox->free = true;                         // Mark as free
    secondBox->next = block_ptr->next;              // Link the rest of the list

    // Update original block to reflect the allocated block
    block_ptr->size = total_size;
    block_ptr->next = secondBox;
    block_ptr->free = false;
    return (void*)(block_ptr + 1);
}

bool check_heap_integrity() {

    BlockHeader* visited[HEAP_CAPACITY / sizeof(BlockHeader)];
    size_t visited_count = 0;

    BlockHeader* curr_block = first_block;
    void* heap_start = (void*)first_block;
    void* heap_end = heap_start + HEAP_CAPACITY;

    while (curr_block != NULL) {
        // Check if we've already seen this block (cycle)
        for (size_t i = 0; i < visited_count; ++i) {
            if (visited[i] == curr_block) {
                return false;
            }
        }

        // Store current block
        if (visited_count < HEAP_CAPACITY / sizeof(BlockHeader)) {
            visited[visited_count++] = curr_block;
        }
        else {
            return false;
        }

        // Check size
        if (curr_block->size == 0 || curr_block->size % ALIGNMENT != 0) {
            return false;
        }

        // Check block boundaries
        void* curr_block_end = (void*)curr_block + curr_block->size;
        if ((void*)curr_block < heap_start || curr_block_end > heap_end) {
            return false;
        }

        // Check for adjacent free blocks (these should have been coalesced)
        if (curr_block->free == true && curr_block->next->free == true) {
            return false;
        }
        curr_block = curr_block->next;
    }
    return true;
}

bool validate_pointer(void* ptr) {
    uintptr_t start = (uintptr_t) heap;
    uintptr_t end   = (uintptr_t) heap_size;
    uintptr_t p     = (uintptr_t) ptr;
    return (p > start && p <= start + end);
}

size_t get_alloc_count() {
    size_t count = 0;
    BlockHeader* curr_block = first_block;
    while (curr_block != NULL) {
        if (curr_block->free == false) {
            count++;
        }
        curr_block = curr_block->next;
    }
    return count;
}

size_t get_free_block_count() {
    size_t count = 0;
    BlockHeader* curr_block = first_block;
    while (curr_block != NULL) {
        if (curr_block->free == true) {
            count++;
        }
        curr_block = curr_block->next;
    }
    return count;
}

size_t get_used_heap_size() {
    size_t size = 0;
    BlockHeader* curr_block = first_block;
    while (curr_block != NULL) {
        size += curr_block->size;
        curr_block = curr_block->next;
    }
    return size;
}


void print_heap() {
    BlockHeader* curr = first_block;
    int i = 0;
    printf("Heap Layout:\n");
    while (curr) {
        printf("Block %d:\n", i++);
        printf("  Block Header Address: %p\n", (void*)curr);
        printf("  Block Total Size: %zu bytes\n", curr->size);
        printf("  Block Data Size: %zu bytes\n", curr->size - sizeof(BlockHeader));
        printf("  Block State: %s\n", curr->free ? "Free" : "Allocated");

        // If the block is free, we can indicate this explicitly
        if (curr->free) {
            printf("  This block is free.\n");
        } else {
            printf("  This block is allocated.\n");
        }

        printf("\n");
        curr = curr->next;
    }
    printf("End of Heap\n");
}
