#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>
#include <stdbool.h>

// Total capacity of simulated heap (in bytes)
#define HEAP_CAPACITY 640000

// Memory alignment boundary (in bytes)
#define ALIGNMENT 16

/**
 * @brief HEader for each memory block in the heap.
 *
 * Each block will contain metadata about the size, whether it's free,
 * and a pointer to the next block in the heap.
 */
typedef struct BlockHeader {
    size_t size;
    bool free;
    struct BlockHeader* next;
} BlockHeader;

/**
 * @brief Aligns the given size to the nearest multiple of ALIGNMENT.
 *
 * @param alloc_size The original size to be aligned.
 *
 * @return The aligned size.
 */
size_t align(size_t alloc_size);

/**
 * @brief Alloates memory from the heap.
 *
 * Uses a first-fit strategy to find a suitable free block, or extends the heap if
 * no such block is found
 *
 * @param requested_bytes Number of bytes requested
 *
 * @return Pointer to the allocated memory, or NULL if allocation fails.
 */
void* heap_alloc(size_t requested_bytes);

/**
 * @brief Frees a previously allocated memory block.
 *
 * Coalesces adjacent free blocks inorder to reduce fragmentation.
 *
 * @param ptr Pointer to the block to be freed.
 *
 * @return void
 */
void heap_free(void* ptr);

/**
 * @brief Resizes an existing memory block in the heap.
 *
 * Will attempt to expand the block in-place if possible. If not, will allocate
 * a new block, copies the data, and frees the old block.
 *
 * @param ptr      Pointer to the block to resize.
 * @param new_size New requested size in bytes
 *
 * @return Pointer to the resized memory block.
 */
void* heap_realloc(void* ptr, size_t new_size);

/**
 * @brief Splits a large block into an allocated block and a smaller free block.
 *
 * Used internally when a block is larger than needed, will leave the remainder as a new free block.
 *
 * @param block_ptr  Pointer to the block to split.
 * @param total_size The desired size for the allocated portion (including the header).
 *
 * @return Pointer to the start of the allocated portion.
 */
void* split_block(BlockHeader* block_ptr, size_t total_size);

/**
 * @brief Prints the current layout of the heap.
 *
 * Useful for debugging and viusalizing how the memory is allocated and freed.
 */
void print_heap();

#endif // ALLOCATOR_H
