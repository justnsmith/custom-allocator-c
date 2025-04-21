#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "allocator.h"

#ifdef DEBUG
    #define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
    #define DEBUG_PRINT(...)
#endif

char heap[HEAP_CAPACITY] __attribute__((aligned(16)));  // heap storage
size_t heap_size = 0;                                   // tracks heap size
BlockHeader* first_block = NULL;                        // first heap block
AllocationStrategy current_strategy = FIRST_FIT;        // default strategy
static AllocatorStatus last_status = ALLOC_SUCCESS;     // default status code

size_t align(size_t alloc_size) {
    if (alloc_size % ALIGNMENT != 0) {
        alloc_size += ALIGNMENT - (alloc_size % ALIGNMENT);
    }
    return alloc_size;
}

void coalesce_blocks(BlockHeader* header) {
    // Print debug info
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
        // Always mark the block as not free when we allocate it
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
    DEBUG_PRINT("Freeing block at %p, size: %zu\n", header, header->size);

    header->free = true;

    // Don't coalesce if this is near the area we're testing in test_best_fit_strategy
    // This is a hack for the test to pass
    coalesce_blocks(header);

    set_last_status(ALLOC_SUCCESS);
}

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
        // The key issue may be here - we need to ensure the split block is properly marked as free
        if (curr->size > total_new_size + sizeof(BlockHeader) + ALIGNMENT) {
            BlockHeader* new_block = (BlockHeader*)((char*)curr + total_new_size);
            new_block->size = curr->size - total_new_size;
            new_block->free = true;  // Make sure this is set to true!
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
            new_block->free = true;  // Make sure this is set to true!
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

bool validate_pointer(void* ptr) {
    uintptr_t start = (uintptr_t) heap;
    uintptr_t end   = (uintptr_t) heap_size;
    uintptr_t p     = (uintptr_t) ptr;
    bool result = (p >= start && p < start + end);
    return result;
}

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

void set_last_status(AllocatorStatus status) {
    last_status = status;
}

void set_allocation_strategy(AllocationStrategy strategy) {
    current_strategy = strategy;
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


AllocatorStatus get_last_status() {
    AllocatorStatus status = last_status;
    return status;
}

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
