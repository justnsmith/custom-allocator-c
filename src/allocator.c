#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "allocator.h"

char heap[HEAP_CAPACITY] = {0};                  // heap storage
size_t heap_size = 0;                            // tracks heap size
BlockHeader* first_block = NULL;                 // first heap block
AllocationStrategy current_strategy = FIRST_FIT; // default strategy

/**
 * @brief Aligns a given memory size to the nearest multiple of the alignment value.
 *
 * This function will round up the provided 'alloc_size' to the next multiple of the
 * specified alignment value (16 bytes). It ensures that the memory block is properly
 * aligned for optimal memory access and system requirements.
 *
 * @param alloc_size The size of the memory block (including the BlockHeader size) that
 *                   needs to be aligned.
 *
 * @return The adjusted size, rounded up to the nearest multiple of the alignment.
 *         If the size is already aligned, no changes will be made and will just
 *         return original 'alloc_size'.
 */
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

/**
 * @brief Allocates the requested number of bytes onto the heap if there is room.
 *
 * This function uses a first-fit approach to allocate memory on the heap. It will first
 * attempt to reuse an existing free block if it meets the size requirements or by creating
 * a new block if no suitable free blocks are available. It will also split large blocks
 * inorder to accomodate the requested size if necessary. If a new block is created, it
 * is added to the end of the heap memory list, and the heap's total size will be updated.
 *
 * @param requested_bytes The number of bytes requested for allocation.
 *
 * @return A void pointer to the allocated memory block, or NULL if the allocation fails.
 *         The pointer returned points to the memory which is just after that BlockHeader
 *
 * @note This function ensures that the allocated block is properly aligned to the system's
 *       alignment boundary (e.g. 16 bytes) and will update the heap's total size.
 *       It also does error checking to ensure that there is enough space in the heap for the
 *       allocation request.
 */
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

/**
 * @brief Frees the specified memory block from the heap and coalesces adjacent free blocks.
 *
 * This function will free the specified memory block from the heap, given that the block
 * is part of the heap. Once the block has been freed, the function will attempt to coalesce
 * the block with adjacent free blocks, both forward (next block) and backward (previous block).
 * If adjacent blocks are free, then they will be merged into a single larger free block with the goal
 * of reducing fragmentation.
 *
 * @param ptr A pointer to a memory block in the heap to be freed.
 *
 * @return void This function does not return any value
 *
 * @note This function only works on a pointer that exists within the heap. If the pointer is not
 *       within the heap's memory range, the program will assert.
 *
 */
void heap_free(void* ptr) {
    assert(ptr != NULL);

    // Check if the pointer is within the valid heap range
    assert((uintptr_t)ptr >= (uintptr_t)heap && (uintptr_t)ptr < (uintptr_t)(heap + HEAP_CAPACITY));

    BlockHeader* header = (BlockHeader*) ptr - 1;
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

/**
 * @brief Will resize a previously allocated memory block to a new size.
 *
 * This function will attempt to resize an existing memory block. If the block is large enough
 * fit the new size, it will split (if possible). If the block can't be resized in-place, then will
 * attempt to merge adjacent free blocks inorder to satisfy the new size requirement. If this isn't
 * possible, then a new block will be allocated, the old block's data will be copied to the new block
 * and the old block will be freed.
 *
 * @param ptr A pointer to the memory block to resize. If NULL, a new block will be allocated.
 * @param new_size The new size (in bytes) that the block should be resized to.
 *
 * @return A pointer to the resized memory block, or NULL is the allocation fails. If the allocatin is
 *         successful, the returned pointer will point to the same memory location as the original pointer,
 *         or a newly allocated block if necessary.
 *
 * @note If the new size is 0, the bolck will be freed and the function will return NULL. The function will also
 *       assert if the pointer does not belong to the heap or is invalid.
 */
void* heap_realloc(void* ptr, size_t new_size) {
    if (ptr == NULL) {
        return heap_alloc(new_size);
    }

    if (new_size == 0) {
        heap_free(ptr);
        return NULL;
    }

     // Check if the pointer is within the valid heap range.
     assert((uintptr_t)ptr >= (uintptr_t)heap && (uintptr_t)ptr < (uintptr_t)(heap + HEAP_CAPACITY));

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

/**
 * @brief Splits a large free memory block into two smaller blocks.
 *
 * This function takes a free block that is larger than the requested size and will split it into two.
 * The first part will be allocated with the requested total size.
 * The second part will stay freed, the size will be whatever remains, and will be added to heap's linked list
 *
 * @param block_ptr Pointer to the free block to split.
 * @param total_size The total size (including header) requested for allocation.
 *
 * @return A pointer to te start of the alloacted portion (just after the block header).
 *
 * @note This function assumes that the original block is large enough to be split (i.e., the remainder
 *       after splitting is still large enough to hold a valid block with header + alignment space).
 *       It also asserts that the block is within the bounds of the heap.
 */
void* split_block(BlockHeader* block_ptr, size_t total_size) {
    // Safety checs to make sure valid input and that the block can be split
    assert(block_ptr != NULL);
    assert((char*)block_ptr >= (char*)first_block && (char*)block_ptr < heap + heap_size);
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

void print_heap() {
    BlockHeader* curr = first_block;
    int i = 0;
    while (curr) {
        printf("Block %d — Size: %zu | Free: %d | Addr: %p\n", i++, curr->size, curr->free, (void*) curr);
        curr = curr->next;
    }
}
