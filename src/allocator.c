#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "allocator.h"

char heap[HEAP_CAPACITY] = {0};                  // heap storage
size_t heap_size = 0;                            // tracks heap size
BlockHeader* first_block = NULL;                 // first heap block
AllocationStrategy current_strategy = FIRST_FIT; // default strategy
pthread_mutex_t heap_mutex = PTHREAD_MUTEX_INITIALIZER;
static AllocatorStatus last_status = ALLOC_SUCCESS;

size_t align(size_t alloc_size) {
    if (alloc_size % ALIGNMENT != 0) {
        alloc_size += ALIGNMENT - (alloc_size % ALIGNMENT);
    }
    return alloc_size;
}

void set_last_status(AllocatorStatus status) {
    last_status = status;
}

AllocatorStatus get_last_status() {
    return last_status;
}

void* find_fit_first(size_t requested_size) {
    BlockHeader* curr_block = first_block;
    while (curr_block != NULL) {
        if (curr_block->free == true && curr_block->size >= requested_size) {
            set_last_status(ALLOC_SUCCESS);
            return (void *)(curr_block + 1);
        }
        curr_block = curr_block->next;
    }
    set_last_status(ALLOC_OUT_OF_MEMORY);
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
    if (best_block != NULL) {
        set_last_status(ALLOC_SUCCESS);
        return (void*)(best_block + 1);
    }
    else {
        set_last_status(ALLOC_OUT_OF_MEMORY);
        return NULL;
    }
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
    if (worst_block != NULL) {
        set_last_status(ALLOC_SUCCESS);
        return (void*)(worst_block + 1);
    }
    else {
        set_last_status(ALLOC_OUT_OF_MEMORY);
        return NULL;
    }
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
        default:
            set_last_status(ALLOC_ERROR);
            return NULL;
    }

    if (found != NULL) {
        BlockHeader* block = (BlockHeader*) found - 1;

        if (block->size >= total_size + sizeof(BlockHeader) + ALIGNMENT) {
            split_block(block, total_size);
        }
        else {
            block->free = false;
        }

        set_last_status(ALLOC_SUCCESS);

        printf("Reused block at %p (%zu bytes)\n", block, block->size);
        return (void*)(block + 1);
    }

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

    if (((uintptr_t) result % ALIGNMENT) != 0) {
        set_last_status(ALLOC_ALIGNMENT_ERROR);
        return NULL;
    }

    set_last_status(ALLOC_SUCCESS);

    printf("Allocated new block of %zu bytes at %p\n", total_size, result);

    return (void*)(new_block + 1);
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

    BlockHeader* header = (BlockHeader*) ptr - 1;
    header->free = true;
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

    BlockHeader* curr = (BlockHeader*) ptr - 1;
    size_t total_new_size = align(new_size + sizeof(BlockHeader));

    // If current block is large enough to fit new size, split the block if possible.
    if (curr->size >= total_new_size) {
        split_block(curr, total_new_size);
        set_last_status(ALLOC_SUCCESS);
        return ptr;
    }

    // If the next block is free and large enough to fit the new size, coalesce the blocks.
    if (curr->next != NULL && curr->next->free == true && (curr->next->size + curr->size) >= total_new_size) {
        curr->size += curr->next->size + sizeof(BlockHeader);
        curr->next = curr->next->next;
        split_block(curr, total_new_size);
        set_last_status(ALLOC_SUCCESS);
        return ptr;
    }

    // If the block cannot be resized in place, allocate a new block and copy data from the old block.
    void* new_ptr = heap_alloc(new_size);
    if (new_ptr == NULL) {
        set_last_status(ALLOC_OUT_OF_MEMORY);
        return NULL;
    }

    size_t copy_size = curr->size - sizeof(BlockHeader);
    if (new_size < copy_size) {
        copy_size = new_size;
    }

    memcpy(new_ptr, ptr, copy_size);
    heap_free(ptr);
    set_last_status(ALLOC_SUCCESS);
    return new_ptr;
}

void* thread_safe_alloc(size_t requested_bytes) {
    pthread_mutex_lock(&heap_mutex);
    void* ptr = heap_alloc(requested_bytes);
    pthread_mutex_unlock(&heap_mutex);
    return ptr;
}

void thread_safe_free(void* ptr) {
    pthread_mutex_lock(&heap_mutex);
    heap_free(ptr);
    pthread_mutex_unlock(&heap_mutex);
}

void* thread_safe_realloc(void* ptr, size_t new_size) {
    pthread_mutex_lock(&heap_mutex);
    void* new_ptr = heap_realloc(ptr, new_size);
    pthread_mutex_unlock(&heap_mutex);
    return new_ptr;
}

void* split_block(BlockHeader* block_ptr, size_t total_size) {
    if (block_ptr == NULL || validate_pointer((void*)(block_ptr + 1)) == false) {
        set_last_status(ALLOC_INVALID_OPERATION);
        return NULL;
    }

    if (block_ptr->size < total_size + sizeof(BlockHeader) + ALIGNMENT) {
        set_last_status(ALLOC_ERROR);
        return NULL;
    }

    // Calculate the address of the new free block after the allocated block
    BlockHeader* secondBox = (BlockHeader*)((char*) block_ptr + total_size);
    secondBox->size = block_ptr->size - total_size; // Remaining size
    secondBox->free = true;                         // Mark as free
    secondBox->next = block_ptr->next;              // Link the rest of the list

    // Update original block to reflect the allocated block
    block_ptr->size = total_size;
    block_ptr->next = secondBox;
    block_ptr->free = false;

    set_last_status(ALLOC_SUCCESS);
    return (void*)(block_ptr + 1);
}

bool check_heap_integrity() {
    pthread_mutex_lock(&heap_mutex);

    BlockHeader* visited[HEAP_CAPACITY / sizeof(BlockHeader)];
    size_t visited_count = 0;

    BlockHeader* curr_block = first_block;
    void* heap_start = (void*)first_block;
    void* heap_end = heap_start + HEAP_CAPACITY;

    while (curr_block != NULL) {
        // Check if we've already seen this block (cycle)
        for (size_t i = 0; i < visited_count; ++i) {
            if (visited[i] == curr_block) {
                pthread_mutex_unlock(&heap_mutex);
                set_last_status(ALLOC_HEAP_ERROR);
                return false;
            }
        }

        // Store current block
        if (visited_count < HEAP_CAPACITY / sizeof(BlockHeader)) {
            visited[visited_count++] = curr_block;
        }
        else {
            pthread_mutex_unlock(&heap_mutex);
            set_last_status(ALLOC_HEAP_ERROR);
            return false;
        }

        // Check size
        if (curr_block->size == 0 || curr_block->size % ALIGNMENT != 0) {
            pthread_mutex_unlock(&heap_mutex);
            set_last_status(ALLOC_ALIGNMENT_ERROR);
            return false;
        }

        // Check block boundaries
        void* curr_block_end = (void*)curr_block + curr_block->size;
        if ((void*)curr_block < heap_start || curr_block_end > heap_end) {
            pthread_mutex_unlock(&heap_mutex);
            set_last_status(ALLOC_HEAP_ERROR);
            return false;
        }

        // Check for adjacent free blocks (these should have been coalesced)
        if (curr_block->free == true && curr_block->next->free == true) {
            pthread_mutex_unlock(&heap_mutex);
            set_last_status(ALLOC_HEAP_ERROR);
            return false;
        }
        curr_block = curr_block->next;
    }
    pthread_mutex_unlock(&heap_mutex);
    set_last_status(ALLOC_HEAP_OK);
    return true;
}

bool validate_pointer(void* ptr) {
    uintptr_t start = (uintptr_t) heap;
    uintptr_t end   = (uintptr_t) heap_size;
    uintptr_t p     = (uintptr_t) ptr;
    return (p >= start && p < start + end);
}

void defragment_heap() {
    pthread_mutex_lock(&heap_mutex);
    BlockHeader* curr_block = first_block;

    while (curr_block != NULL && curr_block->next != NULL) {
        if (curr_block->free == true && curr_block->next->free == true) {
            coalesce_blocks(curr_block);
        }
        else {
            curr_block = curr_block->next;
        }
    }
    pthread_mutex_unlock(&heap_mutex);
}

size_t get_alloc_count() {
    pthread_mutex_lock(&heap_mutex);
    size_t count = 0;
    BlockHeader* curr_block = first_block;
    while (curr_block != NULL) {
        if (curr_block->free == false) {
            count++;
        }
        curr_block = curr_block->next;
    }
    pthread_mutex_unlock(&heap_mutex);
    return count;
}

size_t get_free_block_count() {
    pthread_mutex_lock(&heap_mutex);

    size_t count = 0;
    BlockHeader* curr_block = first_block;
    while (curr_block != NULL) {
        if (curr_block->free == true) {
            count++;
        }
        curr_block = curr_block->next;
    }
    pthread_mutex_unlock(&heap_mutex);
    return count;
}

size_t get_used_heap_size() {
    pthread_mutex_lock(&heap_mutex);

    size_t size = 0;
    BlockHeader* curr_block = first_block;
    while (curr_block != NULL) {
        size += curr_block->size;
        curr_block = curr_block->next;
    }
    pthread_mutex_unlock(&heap_mutex);
    return size;
}

size_t get_free_heap_size() {
    pthread_mutex_lock(&heap_mutex);

    size_t size = 0;
    BlockHeader* curr_block = first_block;
    while (curr_block != NULL) {
        if (curr_block->free == true) {
            size += curr_block->size;
        }
        curr_block = curr_block->next;
    }
    pthread_mutex_unlock(&heap_mutex);
    return size;
}

double get_fragmentation_ratio() {
    pthread_mutex_lock(&heap_mutex);

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

    pthread_mutex_unlock(&heap_mutex);

    if (free_block_count == 0 || total_free_size == 0) {
        return 0.0;
    }

    double avg_free_block_size = (double)total_free_size / free_block_count;
    return avg_free_block_size / total_free_size;
}

void print_heap() {
    pthread_mutex_lock(&heap_mutex);
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

    pthread_mutex_unlock(&heap_mutex);
}

void save_heap_state(const char* filename) {
    FILE *fptr;
    fptr = fopen(filename, "w");

    if (fptr == NULL) {
        fprintf(stderr, "Error: Unable to open file: %s for writing.\n", filename);
        return;
    }

    pthread_mutex_lock(&heap_mutex);

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

    pthread_mutex_unlock(&heap_mutex);

    fclose(fptr);
}

void export_heap_json(const char* filename) {
    FILE* fptr = fopen(filename, "w");
    if (fptr == NULL) {
        fprintf(stderr, "Error: Unable to open file: %s for writing.\n", filename);
        return;
    }

    pthread_mutex_lock(&heap_mutex);

    fprintf(fptr, "{\n");
    fprintf(fptr, " . \"heap_layout\": [\n");

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

    pthread_mutex_unlock(&heap_mutex);

    fclose(fptr);
}
