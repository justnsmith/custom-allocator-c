/**
 * @file allocator.h
 * @brief Header file for a simple memory allocator.
 *
 * This file defines the structures, enums, and function prototypes for a custom memory allocator.
 */

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>
#include <stdbool.h>

// Total capacity of simulated heap (in bytes)
#define HEAP_CAPACITY 640000

// Memory alignment boundary (in bytes)
#define ALIGNMENT 16

/**
 * BlockHeader represents a single block of memory in the heap.
 */
typedef struct BlockHeader {
    size_t size;                // Size of the block (including header)
    bool free;                  // Is the block allocated?
    struct BlockHeader* next;   // Pointer to the next block in the list
} BlockHeader;

/**
 * Allocation strategies supported by the allocator.
 */
typedef enum {
    FIRST_FIT,
    BEST_FIT,
    WORST_FIT,
} AllocationStrategy;

/**
 * Status codes for allocation operations.
 */
typedef enum {
    ALLOC_SUCCESS = 0,        // Allocation success
    ALLOC_ERROR,              // General allocation failure
    ALLOC_OUT_OF_MEMORY,      // No more memory available
    ALLOC_INVALID_FREE,       // Invalid free operation
    ALLOC_ALIGNMENT_ERROR,    // Alignment issue
    ALLOC_INVALID_OPERATION,  // Invalid pointer or corrupted heap
    ALLOC_HEAP_ERROR,          // General heap corruption
    ALLOC_HEAP_OK,             // Heap Success
} AllocatorStatus;

// Global state variables
extern char heap[HEAP_CAPACITY];
extern BlockHeader* first_block;
extern size_t heap_size;
extern AllocationStrategy current_strategy;

// Internal utilities
size_t align(size_t alloc_size);
void coalesce_blocks(BlockHeader* header);
void* split_block(BlockHeader* block_ptr, size_t total_size);

// Fit strategies
BlockHeader* find_fit_first(size_t requested_size);
BlockHeader* find_fit_best(size_t requested_size);
BlockHeader* find_fit_worst(size_t requested_size);

// Allocation and deallocation
void* heap_alloc(size_t requested_bytes);
void heap_free(void* ptr);
void* heap_realloc(void* ptr, size_t new_size);

// Heap Validation and configuration
bool check_heap_integrity();
bool validate_pointer(void* ptr);
void defragment_heap();
void set_last_status(AllocatorStatus status);
void set_allocation_strategy(AllocationStrategy strategy);

// Heap statistics
size_t get_alloc_count();
size_t get_free_block_count();
size_t get_used_heap_size();
size_t get_free_heap_size();
double get_fragmentation_ratio();
AllocatorStatus get_last_status();

// Debugging and visualization
void print_heap();
void save_heap_state(const char* filename);
void export_heap_json(const char* filename);

#endif // ALLOCATOR_H
