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
#define NUM_THREADS 8
#define ALLOCS_PER_THREAD 100
#define MAX_ALLOC_SIZE 1024

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

void test_allocation_at_capacity() {
    TEST_START();
    reset_allocator();

    // Calculate how much space is needed for one header
    size_t header_size = sizeof(BlockHeader);

    // Try to allocate almost the entire heap
    size_t max_alloc_size = HEAP_CAPACITY - header_size - 16;  // Leave a small buffer
    void* ptr = heap_alloc(max_alloc_size);

    assert(ptr != NULL);
    assert(get_last_status() == ALLOC_SUCCESS);

    // Try to allocate one more byte - should fail
    void* ptr2 = heap_alloc(1);
    assert(ptr2 == NULL);
    assert(get_last_status() == ALLOC_OUT_OF_MEMORY);

    heap_free(ptr);
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
    void* blk2 = heap_alloc(300);     // will be shrunk to create smaller hole
    void* spacer2 = heap_alloc(400);  // spacer to prevent coalescing
    void* blk3 = heap_alloc(400);     // will be freed (400-byte hole)
    void* blk4 = heap_alloc(400);     // stay allocated

    printf("HEAP\n");
    print_heap();

    // Free blk1 and blk3 to create 400-byte hole
    heap_free(blk1);
    heap_free(blk2);
    heap_free(blk3);
    heap_free(blk4);

    printf("HEAP 2\n");
    print_heap();

    // Now allocate 200 bytes — BEST_FIT should choose the smallest suitable hole,
    // which is the ~100-byte leftover from blk2's realloc (likely ~104 due to alignment)
    void* test_blk2 = heap_alloc(200);

    print_heap();

    // Check that test_blk is located in the freed region created by realloc of blk2
    BlockHeader* header = (BlockHeader*)test_blk2 - 1;

    print_heap();

    // Assertion ensures it's within the region formerly held by blk2
    assert((void*)test_blk2 >= (void*)blk2 && (void*)test_blk2 < (void*)spacer2);

    // Cleanup
    heap_free(test_blk2);
    heap_free(spacer1);
    heap_free(spacer2);

    TEST_PASSED();
}

void test_worst_fit_strategy() {
    TEST_START();
    reset_allocator();
    set_allocation_strategy(WORST_FIT);

    // Create holes of different sizes
    void* blk1 = heap_alloc(200);
    void* spacer1 = heap_alloc(400);  // spacer to prevent coalescing
    void* blk2 = heap_alloc(400);
    void* spacer2 = heap_alloc(400);  // spacer to prevent coalescing
    void* blk3 = heap_alloc(600);
    void* spacer3 = heap_alloc(400);  // spacer to prevent coalescing
    void* blk4 = heap_alloc(200);

    // Free to create holes of different sizes
    heap_free(blk1); // 200 byte hole
    heap_free(blk2); // 400 byte hole
    heap_free(blk3); // 600 byte hole
    heap_free(blk4); // 200 byte hole

    // Allocate a small block - worst fit should place it in the largest hole
    void* test_blk = heap_alloc(100);
    BlockHeader* header = (BlockHeader*)test_blk - 1;

    // Check that it chose the worst fit (600 byte hole)
    assert((void*)header >= blk3 && (void*)header < blk4);

    heap_free(spacer1);
    heap_free(spacer2);
    heap_free(spacer3);
    heap_free(test_blk);

    TEST_PASSED();
}

// Block Management Tests
void test_block_splitting() {
    TEST_START();
    reset_allocator();

    // Allocate a large block
    void* ptr1 = heap_alloc(500);
    assert(ptr1 != NULL);

    // Free it
    heap_free(ptr1);

    // Allocate a smaller block - should split the free block
    void* ptr2 = heap_alloc(100);
    assert(ptr2 != NULL);

    // The free block count should be at least 1 (the remainder)
    assert(get_free_block_count() >= 1);

    // There should be one allocated block
    assert(get_alloc_count() == 1);

    heap_free(ptr2);

    TEST_PASSED();
}

void test_block_coalescing() {
    TEST_START();
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
    if (free_count != 1) {
        printf("Expected 1 free block after coalescing, got %zu\n", free_count);
    }

    heap_free(ptr3);

    // Now all blocks should be coalesced
    free_count = get_free_block_count();
    if (free_count != 1) {
        printf("Expected 1 free block after complete coalescing, got %zu\n", free_count);
    }

    TEST_PASSED();
}

void test_defragmentation() {
    TEST_START();
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
    printf("Free blocks before: %zu, after: %zu\n", free_before, free_after);
    assert(free_after <= free_before);

    // Free remaining blocks
    for (int i = 1; i < 20; i += 2) {
        heap_free(ptrs[i]);
    }

    TEST_PASSED();
}

// Multithreading Tests
typedef struct {
    int thread_id;
    int error_count;
    void* allocations[ALLOCS_PER_THREAD];
    size_t alloc_sizes[ALLOCS_PER_THREAD];
} ThreadData;

// Function to fill and verify memory contents to detect corruption
void fill_memory(void* ptr, size_t size, int thread_id) {
    unsigned char fill_value = (unsigned char)(thread_id & 0xFF);
    memset(ptr, fill_value, size);
}

int verify_memory(void* ptr, size_t size, int thread_id) {
    unsigned char fill_value = (unsigned char)(thread_id & 0xFF);
    unsigned char* bytes = (unsigned char*)ptr;
    for (size_t i = 0; i < size; i++) {
        if (bytes[i] != fill_value) {
            return 0; // Memory corruption detected
        }
    }
    return 1; // Memory intact
}

// Thread function for allocation test
void* allocation_thread_func(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    data->error_count = 0;

    for (int i = 0; i < ALLOCS_PER_THREAD; i++) {
        // Randomize allocation size between 32 and MAX_ALLOC_SIZE bytes
        size_t size = 32 + (rand() % (MAX_ALLOC_SIZE - 32 + 1));
        data->alloc_sizes[i] = size;

        // Allocate memory using thread-safe allocation
        void* ptr = thread_safe_alloc(size);
        data->allocations[i] = ptr;

        if (ptr == NULL) {
            printf("Thread %d: allocation %d failed\n", data->thread_id, i);
            data->error_count++;
            continue;
        }

        // Fill memory with a pattern based on thread ID
        fill_memory(ptr, size, data->thread_id);

        // Add small delay to increase chance of thread interleaving
        if (rand() % 10 == 0) {
            struct timespec ts = {0, 100000}; // 100 microseconds
            nanosleep(&ts, NULL);
        }
    }

    // Verify memory contents before returning
    for (int i = 0; i < ALLOCS_PER_THREAD; i++) {
        if (data->allocations[i] != NULL) {
            if (!verify_memory(data->allocations[i], data->alloc_sizes[i], data->thread_id)) {
                printf("Thread %d: memory corruption detected in allocation %d\n",
                       data->thread_id, i);
                data->error_count++;
            }
        }
    }

    return NULL;
}

// Thread function for reallocation test
void* reallocation_thread_func(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    data->error_count = 0;

    // First do initial allocations
    for (int i = 0; i < ALLOCS_PER_THREAD; i++) {
        size_t size = 32 + (rand() % (MAX_ALLOC_SIZE - 32 + 1));
        data->alloc_sizes[i] = size;
        data->allocations[i] = thread_safe_alloc(size);

        if (data->allocations[i] == NULL) {
            printf("Thread %d: initial allocation %d failed\n", data->thread_id, i);
            data->error_count++;
            continue;
        }

        fill_memory(data->allocations[i], size, data->thread_id);
    }

    // Now perform reallocations
    for (int i = 0; i < ALLOCS_PER_THREAD; i++) {
        if (data->allocations[i] == NULL) continue;

        // Verify memory is still intact before reallocation
        if (!verify_memory(data->allocations[i], data->alloc_sizes[i], data->thread_id)) {
            printf("Thread %d: memory corruption detected before realloc %d\n",
                   data->thread_id, i);
            data->error_count++;
        }

        // Choose new size (bigger or smaller)
        size_t new_size;
        if (rand() % 2 == 0) {
            // Grow
            new_size = data->alloc_sizes[i] + (rand() % 512);
        } else {
            // Shrink (but not below 32 bytes)
            new_size = data->alloc_sizes[i] - (rand() %
                      (data->alloc_sizes[i] > 64 ? data->alloc_sizes[i] - 32 : 32));
            if (new_size < 32) new_size = 32;
        }

        // Perform reallocation
        void* new_ptr = thread_safe_realloc(data->allocations[i], new_size);

        if (new_ptr == NULL) {
            printf("Thread %d: reallocation %d failed\n", data->thread_id, i);
            data->error_count++;
            continue;
        }

        // Check if memory data was preserved for the smaller of the two sizes
        size_t check_size = (new_size < data->alloc_sizes[i]) ? new_size : data->alloc_sizes[i];
        if (!verify_memory(new_ptr, check_size, data->thread_id)) {
            printf("Thread %d: memory corruption detected after realloc %d\n",
                   data->thread_id, i);
            data->error_count++;
        }

        // Update the allocation info and fill with the pattern again
        data->allocations[i] = new_ptr;
        data->alloc_sizes[i] = new_size;
        fill_memory(new_ptr, new_size, data->thread_id);

        // Add small delay occasionally
        if (rand() % 10 == 0) {
            struct timespec ts = {0, 100000}; // 100 microseconds
            nanosleep(&ts, NULL);
        }
    }

    return NULL;
}

void test_multithreaded_allocation() {
    TEST_START();
    reset_allocator();

    pthread_t threads[NUM_THREADS];
    ThreadData thread_data[NUM_THREADS];

    // Initialize thread data
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].error_count = 0;
        memset(thread_data[i].allocations, 0, sizeof(thread_data[i].allocations));
    }

    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, allocation_thread_func, &thread_data[i]) != 0) {
            perror("pthread_create failed");
            TEST_FAILED();
            return;
        }
    }

    // Wait for all threads to complete
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Check for errors and free any remaining allocations
    int total_errors = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        total_errors += thread_data[i].error_count;

        // Free allocations
        for (int j = 0; j < ALLOCS_PER_THREAD; j++) {
            if (thread_data[i].allocations[j] != NULL) {
                thread_safe_free(thread_data[i].allocations[j]);
            }
        }
    }

    if (total_errors > 0) {
        printf("Multithreaded allocation test detected %d errors\n", total_errors);
        TEST_FAILED();
    } else {
        TEST_PASSED();
    }
}

void test_multithreaded_reallocation() {
    TEST_START();
    reset_allocator();

    pthread_t threads[NUM_THREADS];
    ThreadData thread_data[NUM_THREADS];

    // Initialize thread data
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].error_count = 0;
        memset(thread_data[i].allocations, 0, sizeof(thread_data[i].allocations));
    }

    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, reallocation_thread_func, &thread_data[i]) != 0) {
            perror("pthread_create failed");
            TEST_FAILED();
            return;
        }
    }

    // Wait for all threads to complete
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Check for errors and free any remaining allocations
    int total_errors = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        total_errors += thread_data[i].error_count;

        // Free allocations
        for (int j = 0; j < ALLOCS_PER_THREAD; j++) {
            if (thread_data[i].allocations[j] != NULL) {
                thread_safe_free(thread_data[i].allocations[j]);
            }
        }
    }

    // Also check heap integrity after multithreaded operations
    if (!check_heap_integrity()) {
        printf("Heap integrity compromised after multithreaded reallocation\n");
        total_errors++;
    }

    if (total_errors > 0) {
        printf("Multithreaded reallocation test detected %d errors\n", total_errors);
        TEST_FAILED();
    } else {
        TEST_PASSED();
    }
}

// Performance Tests
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
        assert(ptrs[i] != NULL);
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
void test_heap_stats() {
    TEST_START();
    reset_allocator();

    // Initial state
    assert(get_alloc_count() == 0);
    assert(get_free_block_count() == 0);

    // Allocate some blocks
    void* ptr1 = heap_alloc(100);
    void* ptr2 = heap_alloc(200);
    void* ptr3 = heap_alloc(300);

    // Check stats
    assert(get_alloc_count() == 3);
    assert(get_free_block_count() == 0);
    assert(get_used_heap_size() > 0);

    // Free a block
    heap_free(ptr2);

    // Check updated stats
    assert(get_alloc_count() == 2);
    assert(get_free_block_count() == 1);
    assert(get_free_heap_size() > 0);

    // Free remaining blocks
    heap_free(ptr1);
    heap_free(ptr3);

    TEST_PASSED();
}

void test_heap_export() {
    TEST_START();
    reset_allocator();

    // Create some heap state
    void* ptr1 = heap_alloc(100);
    void* ptr2 = heap_alloc(200);
    heap_free(ptr1);

    // Export as text
    save_heap_state("heap_state.txt");

    // Export as JSON
    export_heap_json("heap_state.json");

    // Just verify the files were created
    FILE* f1 = fopen("heap_state.txt", "r");
    assert(f1 != NULL);
    fclose(f1);

    FILE* f2 = fopen("heap_state.json", "r");
    assert(f2 != NULL);
    fclose(f2);

    // Clean up
    heap_free(ptr2);

    TEST_PASSED();
}

int main(int argc, char* argv[]) {
    srand(time(NULL));
    printf(ANSI_COLOR_MAGENTA "Starting Memory Allocator Tests\n" ANSI_COLOR_RESET);

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

    // Multithreading Tests
    //test_multithreaded_allocation();    // TODO
    //test_multithreaded_reallocation(); // TODO

    // Performance Tests
    test_allocation_performance();
    compare_allocation_strategies();

    // Utility Function Tests
    test_heap_stats();
    test_heap_export();

    printf(ANSI_COLOR_MAGENTA "\nAll tests completed!\n" ANSI_COLOR_RESET);
    return 0;
}
