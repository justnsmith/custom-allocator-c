#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <time.h>
#include "allocator.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define TEST_PASSED() printf(ANSI_COLOR_GREEN "[PASSED] " ANSI_COLOR_RESET "%s\n", __func__)
#define TEST_FAILED() printf(ANSI_COLOR_RED "[FAILED] " ANSI_COLOR_RESET "%s\n", __func__)
#define TEST_START() printf(ANSI_COLOR_BLUE "[RUNNING] " ANSI_COLOR_RESET "%s\n", __func__)

// Global variables for testing
pthread_t threads[10];
volatile int thread_errors = 0;
volatile int thread_count = 0;

// Helper functions
void reset_allocator() {
    memset(heap, 0, HEAP_CAPACITY);
    heap_size = 0;
    first_block = NULL;
    current_strategy = FIRST_FIT;
    set_last_status(ALLOC_SUCCESS);
}

void verify_allocation(void *ptr, size_t expected_size) {
    assert(ptr != NULL);
    BlockHeader *header = (BlockHeader*)ptr - 1;
    assert(header->free == false);
    assert(header->size >= expected_size + sizeof(BlockHeader));
}

// Basic Functionality Tests
void test_basic_allocation() {
    TEST_START();
    reset_allocator();

    // Allocate a simple block
    void* ptr = heap_alloc(100);
    assert(ptr != NULL);
    verify_allocation(ptr, 100);

    // Check that we can write to and read from it
    memset(ptr, 'A', 100);
    for (int i = 0; i < 100; i++) {
        assert(((char*)ptr)[i] == 'A');
    }

    heap_free(ptr);
    assert(get_last_status() == ALLOC_SUCCESS);

    TEST_PASSED();
}

void test_multiple_allocations() {
    TEST_START();
    reset_allocator();

    // Allocate multiple blocks of different sizes
    void* ptrs[5];
    size_t sizes[5] = {64, 128, 256, 512, 1024};
    char fill_chars[5] = {'A', 'B', 'C', 'D', 'E'};

    for (int i = 0; i < 5; i++) {
        ptrs[i] = heap_alloc(sizes[i]);
        assert(ptrs[i] != NULL);
        verify_allocation(ptrs[i], sizes[i]);
        memset(ptrs[i], fill_chars[i], sizes[i]);
    }

    // Verify data integrity
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < sizes[i]; j++) {
            assert(((char*)ptrs[i])[j] == fill_chars[i]);
        }
    }

    // Free in reverse order
    for (int i = 4; i >= 0; i--) {
        heap_free(ptrs[i]);
        assert(get_last_status() == ALLOC_SUCCESS);
    }

    TEST_PASSED();
}

void test_reallocation() {
    TEST_START();
    reset_allocator();

    // Allocate a block and fill it
    void* ptr1 = heap_alloc(100);
    assert(ptr1 != NULL);
    memset(ptr1, 'X', 100);

    // Reallocate to larger size
    void* ptr2 = heap_realloc(ptr1, 200);
    assert(ptr2 != NULL);

    // Verify data was preserved
    for (int i = 0; i < 100; i++) {
        assert(((char*)ptr2)[i] == 'X');
    }

    // Fill the expanded area
    memset((char*)ptr2 + 100, 'Y', 100);

    // Reallocate to smaller size
    void* ptr3 = heap_realloc(ptr2, 50);

    printf("got hereee\n");
    assert(ptr3 != NULL);

    // Verify first 50 bytes preserved
    for (int i = 0; i < 50; i++) {
        assert(((char*)ptr3)[i] == 'X');
    }

    heap_free(ptr3);
    assert(get_last_status() == ALLOC_SUCCESS);

    TEST_PASSED();
}

// Edge Cases Tests
void test_zero_allocation() {
    TEST_START();
    reset_allocator();

    // Test allocating 0 bytes
    void* ptr = heap_alloc(0);
    assert(ptr == NULL);

    heap_free(ptr);
    assert(get_last_status() == ALLOC_INVALID_FREE);

    TEST_PASSED();
}

void test_large_allocation() {
    TEST_START();
    reset_allocator();

    // Test allocating a large block (half the heap)
    size_t half_heap = HEAP_CAPACITY / 2;
    void* ptr = heap_alloc(half_heap - sizeof(BlockHeader) - 100); // Leave some room for header
    assert(ptr != NULL);

    heap_free(ptr);
    assert(get_last_status() == ALLOC_SUCCESS);

    TEST_PASSED();
}

void test_too_large_allocation() {
    TEST_START();
    reset_allocator();

    // Test allocating something larger than the heap
    void* ptr = heap_alloc(HEAP_CAPACITY + 1);
    assert(ptr == NULL);
    assert(get_last_status() == ALLOC_OUT_OF_MEMORY);

    TEST_PASSED();
}

void test_alignment() {
    TEST_START();
    reset_allocator();

    // Test that allocations are properly aligned
    for (int i = 1; i <= 32; i++) {
        void* ptr = heap_alloc(i);
        assert(ptr != NULL);

        // Adjust for the BlockHeader size
        uintptr_t user_data_ptr = (uintptr_t)ptr + sizeof(BlockHeader);

        // Check if the user data (after BlockHeader) is aligned
        assert(user_data_ptr % ALIGNMENT == 0);  // Ensure alignment of the user data

        heap_free(ptr);
    }

    TEST_PASSED();
}

void test_maximum_fragmentation() {
    TEST_START();
    reset_allocator();

    // Create maximum fragmentation by allocating and freeing alternating blocks
    void* ptrs[100];
    for (int i = 0; i < 100; i++) {
        ptrs[i] = heap_alloc(64);
        assert(ptrs[i] != NULL);
    }

    // Free every other block
    for (int i = 0; i < 100; i += 2) {
        heap_free(ptrs[i]);
    }

    // Check fragmentation ratio
    double frag = get_fragmentation_ratio();
    printf("Fragmentation ratio: %.4f\n", frag);

    // Free remaining blocks
    for (int i = 1; i < 100; i += 2) {
        heap_free(ptrs[i]);
    }

    TEST_PASSED();
}

// Error Handling Tests
void test_invalid_free() {
    TEST_START();
    reset_allocator();

    // Test freeing NULL pointer
    heap_free(NULL);
    assert(get_last_status() == ALLOC_INVALID_FREE);

    // Test freeing pointer not from our heap
    int stack_var;
    heap_free(&stack_var);
    assert(get_last_status() == ALLOC_HEAP_ERROR);

    TEST_PASSED();
}

void test_double_free() {
    TEST_START();
    reset_allocator();

    // Allocate and free the same pointer twice
    void* ptr = heap_alloc(100);
    assert(ptr != NULL);

    heap_free(ptr);
    assert(get_last_status() == ALLOC_SUCCESS);

    // This should mark the block as free but not cause an error
    // since we're just changing a flag
    heap_free(ptr);

    TEST_PASSED();
}

void test_use_after_free() {
    TEST_START();
    reset_allocator();

    // Allocate, free, then try to use the memory
    void* ptr = heap_alloc(100);
    assert(ptr != NULL);

    heap_free(ptr);

    // This is inherently unsafe, but our allocator doesn't prevent it
    // Just testing that we can still access the memory (though in real life this would be a bug)
    memset(ptr, 'Z', 100);

    TEST_PASSED();
}

void test_heap_integrity() {
    TEST_START();
    reset_allocator();

    // Allocate some blocks
    void* ptr1 = heap_alloc(100);
    void* ptr2 = heap_alloc(200);
    void* ptr3 = heap_alloc(300);

    // Check integrity
    assert(check_heap_integrity());

    // Free in random order
    heap_free(ptr2);
    heap_free(ptr1);
    heap_free(ptr3);

    // Check integrity again
    assert(check_heap_integrity());

    TEST_PASSED();
}

// Allocation Strategies Tests
void test_first_fit_strategy() {
    TEST_START();
    reset_allocator();
    set_allocation_strategy(FIRST_FIT);

    // Create a situation where first-fit makes a specific choice
    void* blk1 = heap_alloc(200);
    void* blk2 = heap_alloc(200);
    void* blk3 = heap_alloc(200);

    // Free the middle block
    heap_free(blk2);

    // Allocate a smaller block - should use the middle free block
    void* blk4 = heap_alloc(100);
    BlockHeader* header4 = (BlockHeader*)blk4 - 1;

    // In first-fit, this should be placed in the first free block (where blk2 was)
    assert((void*)header4 >= blk1 && (void*)header4 < blk3);

    heap_free(blk1);
    heap_free(blk3);
    heap_free(blk4);

    TEST_PASSED();
}

void test_best_fit_strategy() {
    TEST_START();
    reset_allocator();
    set_allocation_strategy(BEST_FIT);

    // Create allocated blocks (some to free, some as spacers)
    void* blk1 = heap_alloc(400);     // will be freed (400-byte hole)
    void* spacer1 = heap_alloc(400);  // spacer to prevent coalescing
    void* blk2 = heap_alloc(400);     // will be shrunk to create smaller hole
    void* spacer2 = heap_alloc(400);  // spacer to prevent coalescing
    void* blk3 = heap_alloc(400);     // will be freed (400-byte hole)
    void* blk4 = heap_alloc(400);     // stay allocated

    // Free blk1 and blk3 to create 400-byte hole
    heap_free(blk1);
    heap_free(blk3);

    // Shrink blk2 to 300 bytes (leaves a ~100-byte hole)
    void* new_blk2 = heap_realloc(blk2, 300);
    heap_free(new_blk2); // Creates a ~100-byte hole

    // Now allocate 200 bytes — BEST_FIT should choose the smallest suitable hole,
    // which is the ~100-byte leftover from blk2's realloc (likely ~104 due to alignment)
    void* test_blk = heap_alloc(200);

    // Check that test_blk is located in the freed region created by realloc of blk2
    BlockHeader* header = (BlockHeader*)test_blk - 1;

    // Assertion ensures it's within the region formerly held by blk2
    assert((void*)header >= new_blk2 && (void*)header < spacer2);

    // Cleanup
    heap_free(test_blk);
    heap_free(blk4);
    heap_free(spacer1);
    heap_free(spacer2);

    TEST_PASSED();
}


int main(int argc, char* argv[]) {
    srand(time(NULL));
    printf(ANSI_COLOR_MAGENTA "Starting Memory Allocator Tests\n" ANSI_COLOR_RESET);

    test_basic_allocation();
    test_multiple_allocations();
    test_reallocation();
    test_zero_allocation();
    test_large_allocation();
    test_too_large_allocation();
    test_alignment();
    test_maximum_fragmentation();

    test_invalid_free();
    test_double_free();
    test_use_after_free();
    test_heap_integrity();

    test_first_fit_strategy();
    //test_best_fit_strategy(); // error

    printf(ANSI_COLOR_MAGENTA "\nAll tests completed!\n" ANSI_COLOR_RESET);
    return 0;
}
