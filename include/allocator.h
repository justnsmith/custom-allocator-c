#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>

// Total capacity of simulated heap (in bytes)
#define HEAP_CAPACITY 640000

// Memory alignment boundary (in bytes)
#define ALIGNMENT 16

typedef struct BlockHeader {
    size_t size;
    bool free;
    struct BlockHeader* next;
} BlockHeader;

typedef enum {
    FIRST_FIT,
    BEST_FIT,
    WORST_FIT,
} AllocationStrategy;

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

extern BlockHeader* first_block;
extern size_t heap_size;
extern AllocationStrategy current_strategy;
extern pthread_mutex_t heap_mutex;

size_t align(size_t alloc_size);
void set_last_status(AllocatorStatus status);
AllocatorStatus get_last_status();

void coalesce_blocks(BlockHeader* header);
void* split_block(BlockHeader* block_ptr, size_t total_size);

void* find_fit_first(size_t requested_size);
void* find_fit_best(size_t requested_size);
void* find_fit_worst(size_t requested_size);

void* heap_alloc(size_t requested_bytes);
void heap_free(void* ptr);
void* heap_realloc(void* ptr, size_t new_size);

void* thread_safe_alloc(size_t requested_bytes);
void thread_safe_free(void* ptr);
void* thread_safe_realloc(void* ptr, size_t new_size);

bool check_heap_integrity();
bool validate_pointer(void* ptr);
void defragment_heap();

size_t get_alloc_count();
size_t get_free_block_count();
size_t get_used_heap_size();
size_t get_free_heap_size();
double get_fragmentation_ratio();

void print_heap();
void save_heap_state(const char* filename);
void export_heap_json(const char* filename);

#endif // ALLOCATOR_H
