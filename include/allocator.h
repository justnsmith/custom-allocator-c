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

extern BlockHeader* first_block;
extern size_t heap_size;
extern AllocationStrategy current_strategy;
extern pthread_mutex_t heap_mutex;

size_t align(size_t alloc_size);

void* find_fit_first(size_t requested_size);
void* find_fit_best(size_t requested_size);
void* find_fit_worst(size_t requested_size);

void coalesce_blocks(BlockHeader* header);

void* thread_safe_alloc(size_t requested_bytes);                         // Not done
void* heap_alloc(size_t requested_bytes);

void thread_safe_free(void* ptr);                                        // Not done
void heap_free(void* ptr);

void* thread_safe_realloc(void* ptr, size_t new_size);
void* heap_realloc(void* ptr, size_t new_size);

void* split_block(BlockHeader* block_ptr, size_t total_size);

bool check_heap_integrity();
bool validate_pointer(void* ptr);
void defragment_heap();                                                  // Not done

size_t get_alloc_count();
size_t get_free_block_count();
size_t get_used_heap_size();
void print_heap();

#endif // ALLOCATOR_H
