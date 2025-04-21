/**
 * @file allocator_test.c
 * @brief Unit tests for a simple memory allocator.
 *
 * This file contains a series of unit tests to validate the functionality
 * of a custom memory allocator. The tests cover basic allocation,
 * deallocation, reallocation, and various edge cases. The tests also
 * include performance benchmarks and checks for memory leaks.
 * The allocator supports different allocation strategies (first-fit,
 * best-fit, worst-fit) and provides functions for memory management,
 * including allocation, deallocation, reallocation, and heap integrity checks.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include "allocator.h"

// ANSI color codes for colored console output.
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_RESET   "\x1b[0m"

// Macros for test output formatting.
#define TEST_START() printf(ANSI_COLOR_YELLOW "[STARTING] " ANSI_COLOR_RESET "%s\n", __func__)
#define TEST_PASSED() printf(ANSI_COLOR_GREEN "[PASSED] " ANSI_COLOR_RESET "%s\n", __func__)
#define TEST_FAILED() do { \
    fprintf(stderr, ANSI_COLOR_RED "[FAILED] " ANSI_COLOR_RESET "%s (%s:%d)\n", __func__, __FILE__, __LINE__); \
    return; \
} while (0)

// Helper functions

// Ressets the allocator to a clean inital state.
void reset_allocator() {
    memset(heap, 0, HEAP_CAPACITY);
    heap_size = 0;
    first_block = NULL;
    current_strategy = FIRST_FIT;
    set_last_status(ALLOC_SUCCESS);
}

// Verifies that the given pointer was successfully allocated
// and that the block has the expected size.
bool verify_allocation(void *ptr, size_t expected_size) {
    if (ptr == NULL) return false;

    BlockHeader *header = (BlockHeader*)ptr - 1;

    // CHeck that the block is marked as not free and that it has enough space
    if (header->free != false || header->size < expected_size + sizeof(BlockHeader)) {
        return false;
    }
    else {
        return true;
    }
}

// Basic Functionality Tests

// Test basic allocation, memory access, and deallocation.
void test_basic_allocation() {
    reset_allocator();

    // Allocate a simple block of 100 bytes
    void* ptr = heap_alloc(100);
    if (ptr == NULL) {
        TEST_FAILED();
    }

    if (!verify_allocation(ptr, 100)) TEST_FAILED();

    // Write to entire block and verify contents
    memset(ptr, 'A', 100);
    for (int i = 0; i < 100; i++) {
        if (((char*)ptr)[i] != 'A') TEST_FAILED();
    }

    heap_free(ptr);
    if (get_last_status() != ALLOC_SUCCESS) TEST_FAILED();

    TEST_PASSED();
}

// Test allocatoin of multiple blocks, data integrity, and deallocation.
void test_multiple_allocations() {
    reset_allocator();

    // Allocate multiple blocks of different sizes
    void* ptrs[5];
    size_t sizes[5] = {64, 128, 256, 512, 1024};
    char fill_chars[5] = {'A', 'B', 'C', 'D', 'E'};

    for (int i = 0; i < 5; i++) {
        ptrs[i] = heap_alloc(sizes[i]);
        if (ptrs[i] == NULL) TEST_FAILED();
        if (!verify_allocation(ptrs[i], sizes[i])) TEST_FAILED();
        memset(ptrs[i], fill_chars[i], sizes[i]);
    }

    // Verify data integrity
    for (int i = 0; i < 5; i++) {
        for (size_t j = 0; j < sizes[i]; j++) {
            if (((char*)ptrs[i])[j] != fill_chars[i]) TEST_FAILED();
        }
    }

    // Free in reverse order
    for (int i = 4; i >= 0; i--) {
        heap_free(ptrs[i]);
        if(get_last_status() != ALLOC_SUCCESS) TEST_FAILED();
    }

    TEST_PASSED();
}

// Test allocating a block that nearly fills the heap and make sure no
// over-allocation happens.
void test_allocation_at_capacity() {
    reset_allocator();

    // Calculate how much space is needed for one header
    size_t header_size = sizeof(BlockHeader);

    // Try to allocate almost the entire heap
    size_t max_alloc_size = HEAP_CAPACITY - header_size - 16;  // Leave a small buffer
    void* ptr = heap_alloc(max_alloc_size);

    if (ptr == NULL) TEST_FAILED();
    if (get_last_status() != ALLOC_SUCCESS) TEST_FAILED();

    // Try to allocate one more byte - should fail
    void* ptr2 = heap_alloc(1);
    if (ptr2 != NULL) TEST_FAILED();
    if (get_last_status() != ALLOC_OUT_OF_MEMORY) TEST_FAILED();

    heap_free(ptr);
    TEST_PASSED();
}

// Test reallocting memory blocks to both larger and smaller sizes.
void test_reallocation() {
    reset_allocator();

    // Allocate a block and fill it
    void* ptr1 = heap_alloc(100);
    if (ptr1 == NULL) TEST_FAILED();
    memset(ptr1, 'X', 100);

    // Reallocate to larger size
    void* ptr2 = heap_realloc(ptr1, 200);
    if(ptr2 == NULL) TEST_FAILED();

    // Verify data was preserved
    for (int i = 0; i < 100; i++) {
        if (((char*)ptr2)[i] != 'X') TEST_FAILED();
    }

    // Fill the expanded area
    memset((char*)ptr2 + 100, 'Y', 100);

    // Reallocate to smaller size
    void* ptr3 = heap_realloc(ptr2, 50);

    if (ptr3 == NULL) TEST_FAILED();

    // Verify first 50 bytes preserved
    for (int i = 0; i < 50; i++) {
        if (((char*)ptr3)[i] != 'X') TEST_FAILED();
    }

    heap_free(ptr3);
    if (get_last_status() != ALLOC_SUCCESS) TEST_FAILED();

    TEST_PASSED();
}

// Edge Cases Tests

// Test allocating 0 bytes and freeing a NULL pointer.
void test_zero_allocation() {
    reset_allocator();

    // Test allocating 0 bytes
    void* ptr = heap_alloc(0);
    if (ptr != NULL) TEST_FAILED();

    heap_free(ptr);
    if (get_last_status() != ALLOC_INVALID_FREE) TEST_FAILED();

    TEST_PASSED();
}

// Test allocating a large block (half the heap) and freeing it.
void test_large_allocation() {
    reset_allocator();

    // Test allocating a large block (half the heap)
    size_t half_heap = HEAP_CAPACITY / 2;
    void* ptr = heap_alloc(half_heap - sizeof(BlockHeader) - 100); // Leave some room for header
    if (ptr == NULL) TEST_FAILED();

    heap_free(ptr);
    if (get_last_status() != ALLOC_SUCCESS) TEST_FAILED();

    TEST_PASSED();
}

// Test allocating a block larger than the heap size
void test_too_large_allocation() {
    reset_allocator();

    // Test allocating something larger than the heap
    void* ptr = heap_alloc(HEAP_CAPACITY + 1);
    if (ptr != NULL) TEST_FAILED();
    if (get_last_status() != ALLOC_OUT_OF_MEMORY) TEST_FAILED();

    TEST_PASSED();
}

// Test that the allocator respects alignment requirements.
void test_alignment() {
    reset_allocator();

    // Test that allocations are properly aligned
    for (int i = 1; i <= 32; i++) {
        void* ptr = heap_alloc(i);
        if (ptr == NULL) TEST_FAILED();

        // Adjust for the BlockHeader size
        uintptr_t user_data_ptr = (uintptr_t)ptr + sizeof(BlockHeader);

        // Check if the user data (after BlockHeader) is aligned
        if (user_data_ptr % ALIGNMENT != 0) TEST_FAILED();

        heap_free(ptr);
    }

    TEST_PASSED();
}

// Create high fragmentation and verify fragmentation ratio stays low.
void test_maximum_fragmentation() {
    reset_allocator();

    void* ptrs[100];

    // Allocate 100 blocks of 64 bytes
    for (int i = 0; i < 100; i++) {
        ptrs[i] = heap_alloc(64);
        if (ptrs[i] == NULL) TEST_FAILED();
    }

    // Free every block to simulate fragmentation
    for (int i = 0; i < 100; i += 2) {
        heap_free(ptrs[i]);
    }

    double frag = get_fragmentation_ratio();

    if (frag < 0.0 || frag > 0.1) TEST_FAILED();

    for (int i = 1; i < 100; i += 2) {
        heap_free(ptrs[i]);
    }

    TEST_PASSED();
}

// Error Handling Tests

// Test freeing a NULL pointer and a pointer not from our heap.
void test_invalid_free() {
    reset_allocator();

    // Test freeing NULL pointer
    heap_free(NULL);
    if (get_last_status() != ALLOC_INVALID_FREE) TEST_FAILED();

    // Test freeing pointer not from our heap
    int stack_var;
    heap_free(&stack_var);
    if (get_last_status() != ALLOC_HEAP_ERROR) TEST_FAILED();

    TEST_PASSED();
}

// Test freeing a pointer that was not allocated by our allocator.
void test_double_free() {
    reset_allocator();

    // Allocate and free the same pointer twice
    void* ptr = heap_alloc(100);
    if (ptr == NULL) TEST_FAILED();

    heap_free(ptr);
    if (get_last_status() != ALLOC_SUCCESS) TEST_FAILED();

    heap_free(ptr);
    if (get_last_status() != ALLOC_INVALID_FREE) TEST_FAILED();

    TEST_PASSED();
}

// Test using memory after it has been freed.
void test_use_after_free() {
    reset_allocator();

    // Allocate, free, then try to use the memory
    void* ptr = heap_alloc(100);
    if (ptr == NULL) TEST_FAILED();

    heap_free(ptr);

    // This is inherently unsafe, but our allocator doesn't prevent it
    // Just testing that we can still access the memory (though in real life this would be a bug)
    memset(ptr, 'Z', 100);

    TEST_PASSED();
}

// Test heap integrity after various operations.
void test_heap_integrity() {
    reset_allocator();

    // Allocate some blocks
    void* ptr1 = heap_alloc(100);
    void* ptr2 = heap_alloc(200);
    void* ptr3 = heap_alloc(300);

    // Check integrity
    if (!check_heap_integrity()) TEST_FAILED();

    // Free in random order
    heap_free(ptr2);
    heap_free(ptr1);
    heap_free(ptr3);

    // Check integrity again
    if (!check_heap_integrity()) TEST_FAILED();

    TEST_PASSED();
}

// Allocation Strategies Tests

// Test first-fit strategy by creating a specific allocation pattern.
void test_first_fit_strategy() {
    reset_allocator();
    set_allocation_strategy(FIRST_FIT);

    // Create a situation where first-fit makes a specific choice
    void* ptr1 = heap_alloc(200);
    void* ptr2 = heap_alloc(200);
    void* ptr3 = heap_alloc(200);

    // Free the middle block
    heap_free(ptr2);

    // Allocate a smaller block - should use the middle free block
    void* ptr4 = heap_alloc(100);
    BlockHeader* ptr4_header = (BlockHeader*)ptr4 - 1;

    // In first-fit, this should be placed in the first free block (where blk2 was)
    if ((void*)ptr4_header < ptr1 || (void*)ptr4_header >= ptr3) TEST_FAILED();

    heap_free(ptr1);
    heap_free(ptr3);
    heap_free(ptr4);

    TEST_PASSED();
}

// Test best-fit strategy by creating a specific allocation pattern.
void test_best_fit_strategy() {
    reset_allocator();
    set_allocation_strategy(BEST_FIT);

    // Create allocated blocks (some to free, some as spacers)
    void* ptr1 = heap_alloc(400);     // will be freed (400-byte hole)
    void* spacer1 = heap_alloc(400);  // spacer to prevent coalescing
    void* ptr2 = heap_alloc(300);     // will be shrunk to create smaller hole
    void* spacer2 = heap_alloc(400);  // spacer to prevent coalescing
    void* ptr3 = heap_alloc(400);     // will be freed (400-byte hole)
    void* ptr4 = heap_alloc(400);     // stay allocated

    // Free blk1 and blk3 to create 400-byte hole
    heap_free(ptr1);
    heap_free(ptr2);
    heap_free(ptr3);
    heap_free(ptr4);

    // Now allocate 200 bytes — BEST_FIT should choose the smallest suitable hole,
    // which is the ~100-byte leftover from blk2's realloc (likely ~104 due to alignment)
    void* test_ptr2 = heap_alloc(200);

    // Assertion ensures it's within the region formerly held by blk2
    if ((void*)test_ptr2 < (void*)ptr2 || (void*)test_ptr2 >= (void*)spacer2) TEST_FAILED();

    // Cleanup
    heap_free(test_ptr2);
    heap_free(spacer1);
    heap_free(spacer2);

    TEST_PASSED();
}

// Test worst-fit strategy by creating a specific allocation pattern.
void test_worst_fit_strategy() {
    reset_allocator();
    set_allocation_strategy(WORST_FIT);

    // Create holes of different sizes
    void* ptr1 = heap_alloc(200);
    void* spacer1 = heap_alloc(400);  // spacer to prevent coalescing
    void* ptr2 = heap_alloc(400);
    void* spacer2 = heap_alloc(400);  // spacer to prevent coalescing
    void* ptr3 = heap_alloc(600);
    void* spacer3 = heap_alloc(400);  // spacer to prevent coalescing
    void* ptr4 = heap_alloc(200);

    // Free to create holes of different sizes
    heap_free(ptr1); // 200 byte hole
    heap_free(ptr2); // 400 byte hole
    heap_free(ptr3); // 600 byte hole
    heap_free(ptr4); // 200 byte hole

    // Allocate a small block - worst fit should place it in the largest hole
    void* test_ptr5 = heap_alloc(100);
    BlockHeader* ptr5_header = (BlockHeader*)test_ptr5;

    // Check that it chose the worst fit (600 byte hole)
    if ((void*)ptr5_header < ptr3 || (void*)ptr5_header >= ptr4) TEST_FAILED();

    heap_free(spacer1);
    heap_free(spacer2);
    heap_free(spacer3);
    heap_free(ptr5_header);

    TEST_PASSED();
}

// Block Management Tests

// Test block splitting and ensure that the remaining free block is still valid.
void test_block_splitting() {
    reset_allocator();

    // Allocate a large block
    void* ptr1 = heap_alloc(500);
    if (ptr1 == NULL) TEST_FAILED();

    // Free it
    heap_free(ptr1);

    // Allocate a smaller block - should split the free block
    void* ptr2 = heap_alloc(100);
    if (ptr2 == NULL) TEST_FAILED();

    // The free block count should be at least 1 (the remainder)
    if (get_free_block_count() < 1) TEST_FAILED();

    // There should be one allocated block
    if (get_alloc_count() != 1) TEST_FAILED();

    heap_free(ptr2);

    TEST_PASSED();
}

// Test block coalescing by allocating and freeing blocks in a specific order.
void test_block_coalescing() {
    reset_allocator();

    // Allocate three contiguous blocks
    void* ptr1 = heap_alloc(100);
    void* ptr2 = heap_alloc(100);
    void* ptr3 = heap_alloc(100);

    // Free them in a way that should trigger coalescing
    heap_free(ptr1);
    heap_free(ptr2);

    // There should be one free block (coalesced)
    size_t free_count = get_free_block_count();
    if (free_count != 1) TEST_FAILED();

    heap_free(ptr3);

    // Now all blocks should be coalesced
    free_count = get_free_block_count();
    if (free_count != 1) TEST_FAILED();

    TEST_PASSED();
}

// Test defragmentation by creating a fragmented heap and then defragmenting it.
void test_defragmentation() {
    reset_allocator();

    // Create a highly fragmented heap
    void* ptrs[20];
    for (int i = 0; i < 20; i++) {
        ptrs[i] = heap_alloc(100);
    }

    // Free every other block
    for (int i = 0; i < 20; i += 2) {
        heap_free(ptrs[i]);
    }

    // Count free blocks before defragmentation
    size_t free_before = get_free_block_count();

    // Defragment the heap
    defragment_heap();

    // Count free blocks after defragmentation
    size_t free_after = get_free_block_count();

    // There should be fewer free blocks after defragmentation
    if (free_after > free_before) TEST_FAILED();

    // Free remaining blocks
    for (int i = 1; i < 20; i += 2) {
        heap_free(ptrs[i]);
    }

    TEST_PASSED();
}

// Performance Tests

// Test allocation performance by measuring time taken for multiple allocations and deallocations.
void test_allocation_performance() {
    TEST_START();
    reset_allocator();

    const int num_allocs = 1000;
    void* ptrs[num_allocs];
    clock_t start, end;
    double cpu_time_used;

    // Test allocation speed
    start = clock();
    for (int i = 0; i < num_allocs; i++) {
        ptrs[i] = heap_alloc(64);
        if (ptrs[i] == NULL) TEST_FAILED();
    }
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Time for %d allocations: %f seconds\n", num_allocs, cpu_time_used);

    // Test deallocation speed
    start = clock();
    for (int i = 0; i < num_allocs; i++) {
        heap_free(ptrs[i]);
    }
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Time for %d deallocations: %f seconds\n", num_allocs, cpu_time_used);

    TEST_PASSED();
}

// Compare the performance of different allocation strategies.
void compare_allocation_strategies() {
    TEST_START();
    const int num_trials = 3;
    const int num_allocs = 500;
    const int num_sizes = 5;
    int sizes[num_sizes] = {32, 64, 128, 256, 512};

    double times[3][num_trials]; // [strategy][trial]

    for (int strategy = 0; strategy < 3; strategy++) {
        for (int trial = 0; trial < num_trials; trial++) {
            reset_allocator();

            // Set strategy
            set_allocation_strategy((AllocationStrategy)strategy);

            // Start timing
            clock_t start = clock();

            // Perform mixed allocations and frees
            void* ptrs[num_allocs];
            for (int i = 0; i < num_allocs; i++) {
                int size_idx = rand() % num_sizes;
                ptrs[i] = heap_alloc(sizes[size_idx]);

                // Free some blocks to create fragmentation
                if (i > 10 && rand() % 4 == 0) {
                    int idx_to_free = rand() % i;
                    if (ptrs[idx_to_free] != NULL) {
                        heap_free(ptrs[idx_to_free]);
                        ptrs[idx_to_free] = NULL;
                    }
                }
            }

            // Free remaining blocks
            for (int i = 0; i < num_allocs; i++) {
                if (ptrs[i] != NULL) {
                    heap_free(ptrs[i]);
                }
            }

            // End timing
            clock_t end = clock();
            double time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
            times[strategy][trial] = time_used;
        }
    }

    // Calculate and display average times
    printf("Average performance comparison:\n");
    const char* strategy_names[3] = {"First Fit", "Best Fit", "Worst Fit"};

    for (int strategy = 0; strategy < 3; strategy++) {
        double avg_time = 0;
        for (int trial = 0; trial < num_trials; trial++) {
            avg_time += times[strategy][trial];
        }
        avg_time /= num_trials;

        printf("%s: %.6f seconds\n", strategy_names[strategy], avg_time);
    }

    TEST_PASSED();
}

// Utility Function Tests

// Test heap statistics functions to ensure they return correct values.
void test_heap_stats() {
    reset_allocator();

    // Initial state
    if(get_alloc_count() != 0) TEST_FAILED();
    if(get_free_block_count() != 0) TEST_FAILED();

    // Allocate some blocks
    void* ptr1 = heap_alloc(100);
    void* ptr2 = heap_alloc(200);
    void* ptr3 = heap_alloc(300);

    // Check stats
    if (get_alloc_count() != 3) TEST_FAILED();
    if (get_free_block_count() != 0) TEST_FAILED();
    if (get_used_heap_size() <= 0) TEST_FAILED();

    // Free a block
    heap_free(ptr2);

    // Check updated stats
    if (get_alloc_count() != 2) TEST_FAILED();
    if (get_free_block_count() != 1) TEST_FAILED();
    if (get_free_heap_size() <= 0) TEST_FAILED();

    // Free remaining blocks
    heap_free(ptr1);
    heap_free(ptr3);

    TEST_PASSED();
}

// Test heap export functions to ensure they create valid files.
void test_heap_export() {
    reset_allocator();

    // Create some heap state
    void* ptr1 = heap_alloc(100);
    void* ptr2 = heap_alloc(200);
    heap_free(ptr1);

    // Export as text
    save_heap_state("data/heap_state.txt");

    // Export as JSON
    export_heap_json("data/heap_state.json");

    // Just verify the files were created
    FILE* f1 = fopen("data/heap_state.txt", "r");
    if (f1 == NULL) TEST_FAILED();
    fclose(f1);

    FILE* f2 = fopen("data/heap_state.json", "r");
    if (f2 == NULL) TEST_FAILED();
    fclose(f2);

    // Clean up
    heap_free(ptr2);

    TEST_PASSED();
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    srand(time(NULL));
    printf(ANSI_COLOR_MAGENTA "Starting Memory Allocator Tests\n\n" ANSI_COLOR_RESET);

    // Basic Functionality Tests
    test_basic_allocation();
    test_multiple_allocations();
    test_allocation_at_capacity();
    test_reallocation();

    // Edge Case Tests
    test_zero_allocation();
    test_large_allocation();
    test_too_large_allocation();
    test_alignment();
    test_maximum_fragmentation();

    // Error Handling Tests
    test_invalid_free();
    test_double_free();
    test_use_after_free();
    test_heap_integrity();

    // Alocation Strategies Tests
    test_first_fit_strategy();
    test_best_fit_strategy();
    test_worst_fit_strategy();

    // Block Management Tests
    test_block_splitting();
    test_block_coalescing();
    test_defragmentation();

    // Utility Function Tests
    test_heap_stats();
    test_heap_export();

    // Performance Tests
    test_allocation_performance();
    compare_allocation_strategies();

    printf(ANSI_COLOR_MAGENTA "\nAll tests completed!\n" ANSI_COLOR_RESET);
    return 0;
}
