/**
 * @file allocator.c
 * @brief Implementation of a simple memory allocator.
 *
 * This file contains the implementation of a custom memory allocator that supports
 * various allocation strategies (first-fit, best-fit, worst-fit) and provides
 * functions for memory management, including allocation, deallocation,
 * reallocation, and heap integrity checks.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "allocator.h"

// Debug print macro: will print message if DEBUG is defined
#ifdef DEBUG
    #define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
    #define DEBUG_PRINT(...)
#endif

char heap[HEAP_CAPACITY] __attribute__((aligned(ALIGNMENT)));  // heap storage, aligned to 16 bytes
size_t heap_size = 0;                                          // tracks heap size
BlockHeader* first_block = NULL;                               // first heap block
AllocationStrategy current_strategy = FIRST_FIT;               // default strategy
static AllocatorStatus last_status = ALLOC_SUCCESS;            // default status code

/**
 * @brief Aligns a given size to the nearest multiple of ALIGNMENT (16 bytes).
 *
 * This function ensures that memory allocations meet the alignment requirements by rounding up to
 * the next multiple of ALIGNMENT (16 bytes). This helps avoid issues with misaligned access on
 * some architectures and makes sure that there is compatibility with the memory layout.
 *
 * @param alloc_size The requested allocation size to be aligned.
 *
 * @return The aligned size, which is a multiple of ALIGNMENT.
 */
size_t align(size_t alloc_size) {
    if (alloc_size % ALIGNMENT != 0) {
        alloc_size += ALIGNMENT - (alloc_size % ALIGNMENT);
    }
    return alloc_size;
}

/**
 * @brief Merges adjacent free blocks with the given block.
 *
 * This function checks both forward and backward directions for adjacent free blocks,
 * merging them into a single larger block. This helps reduce fragmentation in the heap.
 *
 * @param header Pointer to the block header to be coalesced.
 *
 * @return void
 */
void coalesce_blocks(BlockHeader* header) {
    DEBUG_PRINT("Attempting to coalesce block at %p, size: %zu\n", header, header->size);

    // Forward Coalescing
    BlockHeader* next = header->next;
    while (next != NULL && next->free == true) {
        DEBUG_PRINT("Found next free block at %p, size: %zu\n", next, next->size);
        header->size += next->size;
        header->next = next->next;
        DEBUG_PRINT("Coalesced forward, new size: %zu\n", header->size);
        next = header->next;
    }

    // Backward Coalescing
    BlockHeader* curr = first_block;
    BlockHeader* prev = NULL;

    while (curr != NULL && curr != header) {
        prev = curr;
        curr = curr->next;
    }

    if (prev != NULL && prev->free == true) {
        DEBUG_PRINT("Found previous free block at %p, size: %zu\n", prev, prev->size);
        prev->size += header->size;
        prev->next = header->next;
        DEBUG_PRINT("Coalesced backward, new size: %zu\n", prev->size);
    }
}

/**
 * @brief Splits a block into two blocks if the size is large enough.
 *
 * If the block is large enough, it will be split into two parts:
 * The first part will be the requested size, and the second part will
 * be a new free block with the remaining size. Will also update the block linked list.
 *
 * @param block_ptr Pointer to the block header to be split.
 * @param total_size The total size needed for a new block, including the header and alignment.
 *
 * @return Pointer to the newly created block, or NULL if the split was not possible.
 */
void* split_block(BlockHeader* block_ptr, size_t total_size) {
    DEBUG_PRINT("In split_block. Block size: %zu, Total size: %zu\n", block_ptr->size, total_size);

    if (block_ptr == NULL) {
        DEBUG_PRINT("Invalid block pointer.\n");
        set_last_status(ALLOC_INVALID_OPERATION);
        return NULL;
    }

    // Ensure enough space to split
    size_t aligned_size = align(total_size);
    if (block_ptr->size < aligned_size + sizeof(BlockHeader) + ALIGNMENT) {
        DEBUG_PRINT("Not enough space to split the block. Block size: %zu, Total size needed: %zu\n",
               block_ptr->size, aligned_size + sizeof(BlockHeader) + ALIGNMENT);
        set_last_status(ALLOC_ERROR);
        return NULL;
    }

    // Calculate the address of the second block (after the first block's space)
    BlockHeader* secondBox = (BlockHeader*)((char*)block_ptr + aligned_size);

    // Set the size of the second block (remaining space)
    secondBox->size = block_ptr->size - aligned_size;

    if (secondBox->size <= sizeof(BlockHeader)) {
        DEBUG_PRINT("Error: Second block size is too small: %zu\n", secondBox->size);
        set_last_status(ALLOC_ERROR);
        return NULL;
    }

    // Set other properties of the second block
    secondBox->free = true;
    secondBox->next = block_ptr->next;

    // Update the first block
    block_ptr->size = aligned_size;
    block_ptr->next = secondBox;
    block_ptr->free = false;

    DEBUG_PRINT("Second block created at %p with size: %zu\n", secondBox, secondBox->size);
    return (void*)((char*)block_ptr + sizeof(BlockHeader));
}

/**
 * @brief Finds the first free block that fits the requested size.
 *
 * This function traverses the linked list of blocks and returns the first block
 * that is free and large enough to accommodate the requested size.
 *
 * @param requested_size The size of memory requested by the user.
 *
 * @return Pointer to the first suitable block, or NULL if no suitable block is found.
 */
BlockHeader* find_fit_first(size_t requested_size) {
    BlockHeader* curr_block = first_block;
    while (curr_block != NULL) {
        if (curr_block->free == true && curr_block->size >= requested_size) {
            set_last_status(ALLOC_SUCCESS);
            return curr_block;
        }
        curr_block = curr_block->next;
    }
    set_last_status(ALLOC_OUT_OF_MEMORY);
    return NULL;
}

/**
 * @brief Finds the best-fitting free block.
 *
 * This function traverses the linked list of blocks and returns the block
 * that is free and has the smallest size that is still larger than or equal to
 * the requested size.
 *
 * @param requested_size The size of memory requested by the user.
 *
 * @return Pointer to the best-fitting block, or NULL if no suitable block is found.
 */
BlockHeader* find_fit_best(size_t requested_size) {
    BlockHeader* curr_block = first_block;
    BlockHeader* best_block = NULL;
    size_t best_size = SIZE_MAX;  // Start with maximum possible size

    DEBUG_PRINT("\nLooking for best fit of size %zu\n", requested_size);
    while (curr_block != NULL) {
        DEBUG_PRINT("Examining block at %p, size: %zu, free: %d\n",
               curr_block, curr_block->size, curr_block->free);

        if (curr_block->free && curr_block->size >= requested_size) {
            DEBUG_PRINT("  This block is suitable\n");
            // Find the smallest block that fits
            if (curr_block->size < best_size) {
                best_block = curr_block;
                best_size = curr_block->size;
                DEBUG_PRINT("  New best block found: %p, size: %zu\n", best_block, best_size);
            }
        }
        curr_block = curr_block->next;
    }

    if (best_block != NULL) {
        DEBUG_PRINT("Best fit found: %p, size: %zu\n", best_block, best_block->size);
        set_last_status(ALLOC_SUCCESS);
        return best_block;
    } else {
        DEBUG_PRINT("No suitable block found\n");
        set_last_status(ALLOC_OUT_OF_MEMORY);
        return NULL;
    }
}

/**
 * @brief Finds the worst-fitting free block.
 *
 * This function traverses the linked list of blocks and returns the block
 * that is free and has the largest size that is still larger than or equal to
 * the requested size.
 *
 * @param requested_size The size of memory requested by the user.
 *
 * @return Pointer to the worst-fitting block, or NULL if no suitable block is found.
 */
BlockHeader* find_fit_worst(size_t requested_size) {
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
    if (worst_block != NULL) {
        set_last_status(ALLOC_SUCCESS);
        return worst_block;
    }
    else {
        set_last_status(ALLOC_OUT_OF_MEMORY);
        return NULL;
    }
}

/**
 * @brief Allocates a block of memory from the heap.
 *
 * This function attempts to allocate a block of memory from the heap
 * based on the current allocation strategy (first-fit, best-fit, or worst-fit).
 * If no suitable free block is found, it will extend the heap and allocate a new block
 * if there is enough space.
 *
 * @param requested_bytes The number of bytes to allocate (excluding alignment and header)
 *
 * @return Pointer to the allocated memory block, or NULL if allocation fails.
 */
void* heap_alloc(size_t requested_bytes) {
    // Handle zero-size request
    if (requested_bytes == 0) {
        set_last_status(ALLOC_ERROR);
        return NULL;
    }

    size_t total_size = align(requested_bytes + sizeof(BlockHeader));
    BlockHeader* found = NULL;

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
        default:
            set_last_status(ALLOC_ERROR);
            return NULL;
    }

    if (found != NULL) {
        found->free = false;

        // Split the block if it's large enough
        if (found->size >= total_size + sizeof(BlockHeader) + ALIGNMENT) {
            split_block(found, total_size);
        }

        set_last_status(ALLOC_SUCCESS);
        DEBUG_PRINT("Reused block at %p (%zu bytes)\n", found, found->size);
        return (void*)((char*)found + sizeof(BlockHeader));
    }

    // Need to allocate a new block
    if (heap_size + total_size > HEAP_CAPACITY) {
        set_last_status(ALLOC_OUT_OF_MEMORY);
        return NULL;
    }

    void* result = heap + heap_size; // Address where the new block will be allocated.
    if (((uintptr_t) result % ALIGNMENT) != 0) {
        set_last_status(ALLOC_ALIGNMENT_ERROR);
        return NULL;
    }

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

    set_last_status(ALLOC_SUCCESS);
    DEBUG_PRINT("Allocated new block of %zu bytes at %p\n", total_size, result);
    return (void*)((char*)new_block + sizeof(BlockHeader));
}

/**
 * @brief Frees a previously allocated block of memory.
 *
 * This function marks a blow of memory as free, making it available for
 * future allocations. It will also attempt to coalesce adajacent free blocks
 * to prevent fragmentation.
 *
 * @param ptr Pointer to the block of memory to be freed.
 *
 * @return void
 */
void heap_free(void* ptr) {
    if (ptr == NULL) {
        set_last_status(ALLOC_INVALID_FREE);
        return;
    }

    if (validate_pointer(ptr) == false) {
        set_last_status(ALLOC_HEAP_ERROR);
        return;
    }

    BlockHeader* header = (BlockHeader*)((char*)ptr - sizeof(BlockHeader));
    if (header->free) {
        set_last_status(ALLOC_INVALID_FREE);
        return;
    }

    DEBUG_PRINT("Freeing block at %p, size: %zu\n", header, header->size);

    header->free = true;

    coalesce_blocks(header);

    set_last_status(ALLOC_SUCCESS);
}

/**
 * @brief Resizes a previously allocated block of memory.
 *
 * This function will attempt to resize an existing block of memory to a new size.
 * If the block can be resized in place, it will either split or coalesce
 * adjacent blocks as needed. If the block can't be resized in place, a ne w
 * block will be allocated, the data will be copied over, and the old block will be freed.
 *
 * @param ptr Pointer to the previously allocated block of memory to be resized.
 * @param new_size The new size of the block (in bytes).
 *
 * @return void* Pointer to the resized block of memory, or NULL is an error occured.
 */
void* heap_realloc(void* ptr, size_t new_size) {
    if (ptr == NULL) {
        return heap_alloc(new_size);
    }

    if (new_size == 0) {
        heap_free(ptr);
        return NULL;
    }

    if (validate_pointer(ptr) == false) {
        set_last_status(ALLOC_HEAP_ERROR);
        return NULL;
    }

    BlockHeader* curr = (BlockHeader*)((char*)ptr - sizeof(BlockHeader));
    size_t total_new_size = align(new_size + sizeof(BlockHeader));

    // If current block is large enough to fit new size, split the block if possible.
    if (curr->size >= total_new_size) {
        if (curr->size > total_new_size + sizeof(BlockHeader) + ALIGNMENT) {
            BlockHeader* new_block = (BlockHeader*)((char*)curr + total_new_size);
            new_block->size = curr->size - total_new_size;
            new_block->free = true;
            new_block->next = curr->next;

            curr->size = total_new_size;
            curr->next = new_block;

            DEBUG_PRINT("Split during realloc: created free block at %p with size %zu\n", new_block, new_block->size);
        }

        set_last_status(ALLOC_SUCCESS);
        return ptr;
    }

    // If the next block is free and large enough to fit the new size, coalesce the blocks.
    if (curr->next != NULL && curr->next->free == true &&
        (curr->size + curr->next->size + sizeof(BlockHeader)) >= total_new_size) {

        size_t combined_size = curr->size + curr->next->size + sizeof(BlockHeader);
        curr->size = combined_size;
        curr->next = curr->next->next;

        // Now split if needed
        if (curr->size > total_new_size + sizeof(BlockHeader) + ALIGNMENT) {
            BlockHeader* new_block = (BlockHeader*)((char*)curr + total_new_size);
            new_block->size = curr->size - total_new_size;
            new_block->free = true;
            new_block->next = curr->next;

            curr->size = total_new_size;
            curr->next = new_block;

            DEBUG_PRINT("Split after coalesce in realloc: created free block at %p with size %zu\n", new_block, new_block->size);
        }

        set_last_status(ALLOC_SUCCESS);
        return ptr;
    }

    // If the block cannot be resized in place, allocate a new block and copy data.
    void* new_ptr = heap_alloc(new_size);
    if (new_ptr == NULL) {
        set_last_status(ALLOC_OUT_OF_MEMORY);
        return NULL;
    }

    size_t copy_size = curr->size - sizeof(BlockHeader);  // Old payload size
    if (new_size < copy_size) {
        copy_size = new_size;
    }

    memcpy(new_ptr, ptr, copy_size);
    heap_free(ptr);

    set_last_status(ALLOC_SUCCESS);
    return new_ptr;
}

/**
 * @brief Checks the integrity of the heap.
 *
 * This function will verify the heap's structure for any inconsistencies, such as
 * cycles, alginment issues, out-of-bounds blocks, or adjacent free blocks that should
 * have been coalesced. If any errors are found, the function will return false and set
 * an appropriate error status. Otherwise, it wil return true.
 *
 * @return bool True if the heap is valid, false otherwise.
 */
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
                set_last_status(ALLOC_HEAP_ERROR);
                return false;
            }
        }

        // Store current block
        if (visited_count < HEAP_CAPACITY / sizeof(BlockHeader)) {
            visited[visited_count++] = curr_block;
        }
        else {
            set_last_status(ALLOC_HEAP_ERROR);
            return false;
        }

        // Check size
        if (curr_block->size == 0 || curr_block->size % ALIGNMENT != 0) {
            set_last_status(ALLOC_ALIGNMENT_ERROR);
            return false;
        }

        // Check block boundaries
        void* curr_block_end = (void*)curr_block + curr_block->size;
        if ((void*)curr_block < heap_start || curr_block_end > heap_end) {
            set_last_status(ALLOC_HEAP_ERROR);
            return false;
        }

        // Check for adjacent free blocks (these should have been coalesced)
        if (curr_block->free == true && curr_block->next != NULL && curr_block->next->free == true) {
            set_last_status(ALLOC_HEAP_ERROR);
            return false;
        }
        curr_block = curr_block->next;
    }
    set_last_status(ALLOC_HEAP_OK);
    return true;
}

/**
 * @brief Validates if a pointer is within the heap's allocated memory range.
 *
 * This function will check if the provided pointer is within the range of the heap.
 * It's an internal function that helps ensure that a pointer being freed or accessed is
 * part of the allocated heap memory.
 *
 * @param ptr Pointer to the memory to be validated.
 *
 * @return bool True if the pointer is valid, false otherwise.
 */
bool validate_pointer(void* ptr) {
    uintptr_t start = (uintptr_t) heap;
    uintptr_t end   = (uintptr_t) heap_size;
    uintptr_t p     = (uintptr_t) ptr;
    bool result = (p >= start && p < start + end);
    return result;
}

/**
 * @brief Defragments the heap by coalescing adjacent free blocks.
 *
 * This function will traverse the heap and merge adjacent free blocks
 * into a single larger block. This helps reduce fragmentation and
 * makes better use of the available memory.
 *
 * @return void
 */
void defragment_heap() {
    BlockHeader* curr_block = first_block;

    while (curr_block != NULL && curr_block->next != NULL) {
        if (curr_block->free == true && curr_block->next->free == true) {
            coalesce_blocks(curr_block);
        }
        else {
            curr_block = curr_block->next;
        }
    }
}

/**
 * @brief Sets the last status of the allocator.
 *
 * This function updates the last status code of the allocator.
 *
 * @param status The new status code to set.
 *
 * @return void
 */
void set_last_status(AllocatorStatus status) {
    last_status = status;
}

/**
 * @brief Sets the allocation strategy for the allocator.
 *
 * This function updates the current allocation strategy used by the allocator.
 *
 * @param strategy The new allocation strategy to set.
 *
 * @return void
 */
void set_allocation_strategy(AllocationStrategy strategy) {
    current_strategy = strategy;
}

/**
 * @brief Gets the number of allocated blocks in the heap.
 *
 * This function traverses the heap and counts the number of allocated blocks.
 *
 * @return size_t The number of allocated blocks.
 */
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

/**
 * @brief Gets the number of free blocks in the heap.
 *
 * This function traverses the heap and counts the number of free blocks.
 *
 * @return size_t The number of free blocks.
 */
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

/**
 * @brief Gets the total size of the used heap.
 *
 * This function traverses the heap and calculates the total size of all allocated blocks.
 *
 * @return size_t The total size of used heap (in bytes).
 */
size_t get_used_heap_size() {
    size_t size = 0;
    BlockHeader* curr_block = first_block;
    while (curr_block != NULL) {
        size += curr_block->size;
        curr_block = curr_block->next;
    }
    return size;
}

/**
 * @brief Gets the total size of the free heap.
 *
 * This function traverses the heap and calculates the total size of all free blocks.
 *
 * @return size_t The total size of free heap (in bytes).
 */
size_t get_free_heap_size() {
    size_t size = 0;
    BlockHeader* curr_block = first_block;
    while (curr_block != NULL) {
        if (curr_block->free == true) {
            size += curr_block->size;
        }
        curr_block = curr_block->next;
    }
    return size;
}

/**
 * @brief Gets the fragmentation ratio of the heap.
 *
 * This function calculates the fragmentation ratio of the heap by dividing
 * the average size of free blocks by the total size of free memory.
 *
 * @return double The fragmentation ratio (0.0 if no free blocks).
 */
double get_fragmentation_ratio() {
    size_t free_block_count = 0;
    size_t total_free_size = 0;

    BlockHeader* curr_block = first_block;
    while (curr_block != NULL) {
        if (curr_block->free) {
            free_block_count++;
            total_free_size += curr_block->size;
        }
        curr_block = curr_block->next;
    }

    if (free_block_count == 0 || total_free_size == 0) {
        return 0.0;
    }

    double avg_free_block_size = (double)total_free_size / free_block_count;
    return avg_free_block_size / total_free_size;
}

/**
 * @brief Gets the last status of the allocator.
 *
 * This function retrieves the last status code set by the allocator.
 *
 * @return AllocatorStatus The last status code.
 */
AllocatorStatus get_last_status() {
    AllocatorStatus status = last_status;
    return status;
}

/**
 * @brief Prints the current state of the heap.
 *
 * This function traverses the heap and prints the details of each block,
 * including its block number, address, size, and whether it is free or allocated.
 *
 * @return void
 */
void print_heap() {
    BlockHeader* curr = first_block;
    int i = 0;
    printf("Heap Layout:\n");
    while (curr != NULL) {
        printf("Block %d:\n", i++);
        printf("  Block Header Address: %p\n", (void*)curr);
        printf("  Block Total Size: %zu bytes\n", curr->size);
        printf("  Block Data Size: %zu bytes\n", curr->size - sizeof(BlockHeader));
        printf("  Block State: %s\n", curr->free ? "Free" : "Allocated");
        printf("\n");
        curr = curr->next;
    }
    printf("End of Heap\n");
}

/**
 * @brief Saves the current state of the heap to a file.
 *
 * This function traverses the heap and writes the details of each block
 * to a file, including its block number, address, size, and whether it is free or allocated.
 *
 * @param filename The name of the file to save the heap state.
 *
 * @return void
 */
void save_heap_state(const char* filename) {
    FILE *fptr;
    fptr = fopen(filename, "w");

    if (fptr == NULL) {
        fprintf(stderr, "Error: Unable to open file: %s for writing.\n", filename);
        return;
    }

    BlockHeader* curr = first_block;
    int i = 0;
    fprintf(fptr, "Heap Layout:\n");
    while (curr != NULL) {
        fprintf(fptr, "Block %d:\n", i++);
        fprintf(fptr, "  Block Header Address: %p\n", (void*)curr);
        fprintf(fptr, "  Block Total Size: %zu bytes\n", curr->size);
        fprintf(fptr, "  Block Data Size: %zu bytes\n", curr->size - sizeof(BlockHeader));
        fprintf(fptr, "  Block State: %s\n", curr->free ? "Free" : "Allocated");
        fprintf(fptr, "\n");
        curr = curr->next;
    }
    fprintf(fptr, "End of Heap\n");

    fclose(fptr);
}

/**
 * @brief Exports the current state of the heap to a JSON file.
 *
 * This function traverses the heap and writes the details of each block
 * to a JSON file, including its block number, address, size, and whether it is free or allocated.
 *
 * @param filename The name of the file to save the heap state in JSON format.
 *
 * @return void
 */
void export_heap_json(const char* filename) {
    FILE* fptr = fopen(filename, "w");
    if (fptr == NULL) {
        fprintf(stderr, "Error: Unable to open file: %s for writing.\n", filename);
        return;
    }

    fprintf(fptr, "{\n");
    fprintf(fptr, "  \"heap_layout\": [\n");

    BlockHeader* curr_block = first_block;
    int block_index = 0;

    while (curr_block != NULL) {
        fprintf(fptr, "    {\n");
        fprintf(fptr, "      \"block_index\": %d,\n", block_index++);
        fprintf(fptr, "      \"header_address\": \"%p\",\n", (void*)curr_block);
        fprintf(fptr, "      \"total_size\": %zu,\n", curr_block->size);
        fprintf(fptr, "      \"data_size\": %zu,\n", curr_block->size - sizeof(BlockHeader));
        fprintf(fptr, "      \"state\": \"%s\",\n", curr_block->free ? "Free" : "Allocated");
        fprintf(fptr, "      \"next_block\": \"%p\"\n", (void*)curr_block->next);
        curr_block = curr_block->next;

        if (curr_block != NULL) {
            fprintf(fptr, "    },\n");
        } else {
            fprintf(fptr, "    }\n");
        }
    }

    fprintf(fptr, "  ],\n");

    // Add heap stats
    fprintf(fptr, "  \"heap_stats\": {\n");
    fprintf(fptr, "    \"heap_size\": %zu,\n", heap_size);
    fprintf(fptr, "    \"allocated_blocks\": %zu,\n", get_alloc_count());
    fprintf(fptr, "    \"free_blocks\": %zu,\n", get_free_block_count());
    fprintf(fptr, "    \"used_heap_size\": %zu,\n", get_used_heap_size());
    fprintf(fptr, "    \"free_heap_size\": %zu,\n", get_free_heap_size());
    fprintf(fptr, "    \"fragmentation_ratio\": %.4f\n", get_fragmentation_ratio());
    fprintf(fptr, "  }\n");
    fprintf(fptr, "}\n");

    fclose(fptr);
}
