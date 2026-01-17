/**
 * @file allocator_test_enhanced.c
 * @brief Enhanced unit tests for memory allocator with comprehensive coverage.
 *
 * This file extends the original test suite with additional edge cases,
 * stress tests, and validation scenarios to ensure robust allocator behavior.
 */

#include "allocator.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ANSI color codes for colored console output.
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

// Macros for test output formatting.
#define TEST_START() printf(ANSI_COLOR_YELLOW "[STARTING] " ANSI_COLOR_RESET "%s\n", __func__)
#define TEST_PASSED() printf(ANSI_COLOR_GREEN "[PASSED] " ANSI_COLOR_RESET "%s\n", __func__)
#define TEST_FAILED()                                                                                                                      \
    do {                                                                                                                                   \
        fprintf(stderr, ANSI_COLOR_RED "[FAILED] " ANSI_COLOR_RESET "%s (%s:%d)\n", __func__, __FILE__, __LINE__);                         \
        return;                                                                                                                            \
    } while (0)

// Helper functions
void reset_allocator() {
    memset(heap, 0, HEAP_CAPACITY);
    heap_size = 0;
    first_block = NULL;
    current_strategy = FIRST_FIT;
    set_last_status(ALLOC_SUCCESS);
}

bool verify_allocation(void *ptr, size_t expected_size) {
    if (ptr == NULL)
        return false;
    BlockHeader *header = (BlockHeader *)((char *)ptr - sizeof(BlockHeader));
    if (header->free != false || header->size < expected_size + sizeof(BlockHeader)) {
        return false;
    }
    return true;
}

void test_basic_allocation() {
    reset_allocator();
    void *ptr = heap_alloc(100);
    if (ptr == NULL)
        TEST_FAILED();
    if (!verify_allocation(ptr, 100))
        TEST_FAILED();
    memset(ptr, 'A', 100);
    for (int i = 0; i < 100; i++) {
        if (((char *)ptr)[i] != 'A')
            TEST_FAILED();
    }
    heap_free(ptr);
    if (get_last_status() != ALLOC_SUCCESS)
        TEST_FAILED();
    TEST_PASSED();
}

void test_multiple_allocations() {
    reset_allocator();
    void *ptrs[5];
    size_t sizes[5] = {64, 128, 256, 512, 1024};
    char fill_chars[5] = {'A', 'B', 'C', 'D', 'E'};

    for (int i = 0; i < 5; i++) {
        ptrs[i] = heap_alloc(sizes[i]);
        if (ptrs[i] == NULL)
            TEST_FAILED();
        if (!verify_allocation(ptrs[i], sizes[i]))
            TEST_FAILED();
        memset(ptrs[i], fill_chars[i], sizes[i]);
    }

    for (int i = 0; i < 5; i++) {
        for (size_t j = 0; j < sizes[i]; j++) {
            if (((char *)ptrs[i])[j] != fill_chars[i])
                TEST_FAILED();
        }
    }

    for (int i = 4; i >= 0; i--) {
        heap_free(ptrs[i]);
        if (get_last_status() != ALLOC_SUCCESS)
            TEST_FAILED();
    }
    TEST_PASSED();
}

void test_allocation_at_capacity() {
    reset_allocator();
    size_t header_size = sizeof(BlockHeader);
    size_t max_alloc_size = HEAP_CAPACITY - header_size - 16;
    void *ptr = heap_alloc(max_alloc_size);
    if (ptr == NULL)
        TEST_FAILED();
    if (get_last_status() != ALLOC_SUCCESS)
        TEST_FAILED();

    void *ptr2 = heap_alloc(1);
    if (ptr2 != NULL)
        TEST_FAILED();
    if (get_last_status() != ALLOC_OUT_OF_MEMORY)
        TEST_FAILED();

    heap_free(ptr);
    TEST_PASSED();
}

void test_reallocation() {
    reset_allocator();
    void *ptr1 = heap_alloc(100);
    if (ptr1 == NULL)
        TEST_FAILED();
    memset(ptr1, 'X', 100);

    void *ptr2 = heap_realloc(ptr1, 200);
    if (ptr2 == NULL)
        TEST_FAILED();
    for (int i = 0; i < 100; i++) {
        if (((char *)ptr2)[i] != 'X')
            TEST_FAILED();
    }
    memset((char *)ptr2 + 100, 'Y', 100);

    void *ptr3 = heap_realloc(ptr2, 50);
    if (ptr3 == NULL)
        TEST_FAILED();
    for (int i = 0; i < 50; i++) {
        if (((char *)ptr3)[i] != 'X')
            TEST_FAILED();
    }

    heap_free(ptr3);
    if (get_last_status() != ALLOC_SUCCESS)
        TEST_FAILED();
    TEST_PASSED();
}

void test_zero_allocation() {
    reset_allocator();
    void *ptr = heap_alloc(0);
    if (ptr != NULL)
        TEST_FAILED();
    heap_free(ptr);
    if (get_last_status() != ALLOC_INVALID_FREE)
        TEST_FAILED();
    TEST_PASSED();
}

void test_large_allocation() {
    reset_allocator();
    size_t half_heap = HEAP_CAPACITY / 2;
    void *ptr = heap_alloc(half_heap - sizeof(BlockHeader) - 100);
    if (ptr == NULL)
        TEST_FAILED();
    heap_free(ptr);
    if (get_last_status() != ALLOC_SUCCESS)
        TEST_FAILED();
    TEST_PASSED();
}

void test_too_large_allocation() {
    reset_allocator();
    void *ptr = heap_alloc(HEAP_CAPACITY + 1);
    if (ptr != NULL)
        TEST_FAILED();
    if (get_last_status() != ALLOC_OUT_OF_MEMORY)
        TEST_FAILED();
    TEST_PASSED();
}

void test_maximum_fragmentation() {
    reset_allocator();
    void *ptrs[100];
    for (int i = 0; i < 100; i++) {
        ptrs[i] = heap_alloc(64);
        if (ptrs[i] == NULL)
            TEST_FAILED();
    }
    for (int i = 0; i < 100; i += 2) {
        heap_free(ptrs[i]);
    }
    double frag = get_fragmentation_ratio();
    if (frag < 0.0 || frag > 0.1)
        TEST_FAILED();
    for (int i = 1; i < 100; i += 2) {
        heap_free(ptrs[i]);
    }
    TEST_PASSED();
}

void test_invalid_free() {
    reset_allocator();
    heap_free(NULL);
    if (get_last_status() != ALLOC_INVALID_FREE)
        TEST_FAILED();
    int stack_var;
    heap_free(&stack_var);
    if (get_last_status() != ALLOC_HEAP_ERROR)
        TEST_FAILED();
    TEST_PASSED();
}

void test_double_free() {
    reset_allocator();
    void *ptr = heap_alloc(100);
    if (ptr == NULL)
        TEST_FAILED();
    heap_free(ptr);
    if (get_last_status() != ALLOC_SUCCESS)
        TEST_FAILED();
    heap_free(ptr);
    if (get_last_status() != ALLOC_INVALID_FREE)
        TEST_FAILED();
    TEST_PASSED();
}

void test_use_after_free() {
    reset_allocator();
    void *ptr = heap_alloc(100);
    if (ptr == NULL)
        TEST_FAILED();
    heap_free(ptr);
    memset(ptr, 'Z', 100);
    TEST_PASSED();
}

void test_heap_integrity() {
    reset_allocator();
    void *ptr1 = heap_alloc(100);
    void *ptr2 = heap_alloc(200);
    void *ptr3 = heap_alloc(300);
    if (!check_heap_integrity())
        TEST_FAILED();
    heap_free(ptr2);
    heap_free(ptr1);
    heap_free(ptr3);
    if (!check_heap_integrity())
        TEST_FAILED();
    TEST_PASSED();
}

void test_first_fit_strategy() {
    reset_allocator();
    set_allocation_strategy(FIRST_FIT);
    void *ptr1 = heap_alloc(200);
    void *ptr2 = heap_alloc(200);
    void *ptr3 = heap_alloc(200);
    heap_free(ptr2);
    void *ptr4 = heap_alloc(100);
    BlockHeader *ptr4_header = (BlockHeader *)((char *)ptr4 - sizeof(BlockHeader));
    if ((void *)ptr4_header < ptr1 || (void *)ptr4_header >= ptr3)
        TEST_FAILED();
    heap_free(ptr1);
    heap_free(ptr3);
    heap_free(ptr4);
    TEST_PASSED();
}

void test_best_fit_strategy() {
    reset_allocator();
    set_allocation_strategy(BEST_FIT);
    void *ptr1 = heap_alloc(400);
    void *spacer1 = heap_alloc(400);
    void *ptr2 = heap_alloc(300);
    void *spacer2 = heap_alloc(400);
    void *ptr3 = heap_alloc(400);
    void *ptr4 = heap_alloc(400);
    heap_free(ptr1);
    heap_free(ptr2);
    heap_free(ptr3);
    heap_free(ptr4);
    void *test_ptr2 = heap_alloc(200);
    if ((void *)test_ptr2 < (void *)ptr2 || (void *)test_ptr2 >= (void *)spacer2)
        TEST_FAILED();
    heap_free(test_ptr2);
    heap_free(spacer1);
    heap_free(spacer2);
    TEST_PASSED();
}

void test_block_splitting() {
    reset_allocator();
    void *ptr1 = heap_alloc(500);
    if (ptr1 == NULL)
        TEST_FAILED();
    heap_free(ptr1);
    void *ptr2 = heap_alloc(100);
    if (ptr2 == NULL)
        TEST_FAILED();
    if (get_free_block_count() < 1)
        TEST_FAILED();
    if (get_alloc_count() != 1)
        TEST_FAILED();
    heap_free(ptr2);
    TEST_PASSED();
}

void test_block_coalescing() {
    reset_allocator();
    void *ptr1 = heap_alloc(100);
    void *ptr2 = heap_alloc(100);
    void *ptr3 = heap_alloc(100);
    heap_free(ptr1);
    heap_free(ptr2);
    size_t free_count = get_free_block_count();
    if (free_count != 1)
        TEST_FAILED();
    heap_free(ptr3);
    free_count = get_free_block_count();
    if (free_count != 1)
        TEST_FAILED();
    TEST_PASSED();
}

void test_defragmentation() {
    reset_allocator();
    void *ptrs[20];
    for (int i = 0; i < 20; i++) {
        ptrs[i] = heap_alloc(100);
    }
    for (int i = 0; i < 20; i += 2) {
        heap_free(ptrs[i]);
    }
    size_t free_before = get_free_block_count();
    defragment_heap();
    size_t free_after = get_free_block_count();
    if (free_after > free_before)
        TEST_FAILED();
    for (int i = 1; i < 20; i += 2) {
        heap_free(ptrs[i]);
    }
    TEST_PASSED();
}

void test_heap_stats() {
    reset_allocator();
    if (get_alloc_count() != 0)
        TEST_FAILED();
    if (get_free_block_count() != 0)
        TEST_FAILED();
    void *ptr1 = heap_alloc(100);
    void *ptr2 = heap_alloc(200);
    void *ptr3 = heap_alloc(300);
    if (get_alloc_count() != 3)
        TEST_FAILED();
    if (get_free_block_count() != 0)
        TEST_FAILED();
    if (get_used_heap_size() <= 0)
        TEST_FAILED();
    heap_free(ptr2);
    if (get_alloc_count() != 2)
        TEST_FAILED();
    if (get_free_block_count() != 1)
        TEST_FAILED();
    if (get_free_heap_size() <= 0)
        TEST_FAILED();
    heap_free(ptr1);
    heap_free(ptr3);
    TEST_PASSED();
}

void test_realloc_null_pointer() {
    reset_allocator();
    void *ptr = heap_realloc(NULL, 100);
    if (ptr == NULL)
        TEST_FAILED();
    if (!verify_allocation(ptr, 100))
        TEST_FAILED();
    heap_free(ptr);
    TEST_PASSED();
}

void test_realloc_zero_size() {
    reset_allocator();
    void *ptr = heap_alloc(100);
    if (ptr == NULL)
        TEST_FAILED();
    void *result = heap_realloc(ptr, 0);
    if (result != NULL)
        TEST_FAILED();
    TEST_PASSED();
}

void test_realloc_exact_same_size() {
    reset_allocator();
    void *ptr = heap_alloc(100);
    memset(ptr, 'A', 100);
    void *new_ptr = heap_realloc(ptr, 100);
    if (new_ptr != ptr)
        TEST_FAILED();
    for (int i = 0; i < 100; i++) {
        if (((char *)new_ptr)[i] != 'A')
            TEST_FAILED();
    }
    heap_free(new_ptr);
    TEST_PASSED();
}

void test_realloc_with_adjacent_free() {
    reset_allocator();
    void *ptr1 = heap_alloc(100);
    void *ptr2 = heap_alloc(200);
    heap_free(ptr2);

    memset(ptr1, 'X', 100);
    void *new_ptr = heap_realloc(ptr1, 250);
    if (new_ptr == NULL)
        TEST_FAILED();

    for (int i = 0; i < 100; i++) {
        if (((char *)new_ptr)[i] != 'X')
            TEST_FAILED();
    }
    heap_free(new_ptr);
    TEST_PASSED();
}

void test_realloc_must_relocate() {
    reset_allocator();
    void *ptr1 = heap_alloc(100);
    void *ptr2 = heap_alloc(100);
    void *ptr3 = heap_alloc(500);

    memset(ptr1, 'A', 100);
    void *new_ptr = heap_realloc(ptr1, 400);
    if (new_ptr == NULL)
        TEST_FAILED();

    for (int i = 0; i < 100; i++) {
        if (((char *)new_ptr)[i] != 'A')
            TEST_FAILED();
    }

    heap_free(ptr2);
    heap_free(ptr3);
    heap_free(new_ptr);
    TEST_PASSED();
}

void test_coalesce_three_blocks() {
    reset_allocator();
    void *ptrs[5];
    for (int i = 0; i < 5; i++) {
        ptrs[i] = heap_alloc(100);
    }

    heap_free(ptrs[1]);
    heap_free(ptrs[2]);
    heap_free(ptrs[3]);

    if (get_free_block_count() != 1)
        TEST_FAILED();

    heap_free(ptrs[0]);
    heap_free(ptrs[4]);
    TEST_PASSED();
}

void test_coalesce_all_blocks() {
    reset_allocator();
    void *ptrs[10];
    for (int i = 0; i < 10; i++) {
        ptrs[i] = heap_alloc(50);
    }

    for (int i = 0; i < 10; i++) {
        heap_free(ptrs[i]);
    }

    if (get_free_block_count() != 1)
        TEST_FAILED();
    TEST_PASSED();
}

void test_coalesce_checkerboard_pattern() {
    reset_allocator();
    void *ptrs[8];
    for (int i = 0; i < 8; i++) {
        ptrs[i] = heap_alloc(100);
    }

    for (int i = 0; i < 8; i += 2) {
        heap_free(ptrs[i]);
    }

    size_t free_before = get_free_block_count();

    for (int i = 1; i < 8; i += 2) {
        heap_free(ptrs[i]);
    }

    if (get_free_block_count() != 1)
        TEST_FAILED();
    TEST_PASSED();
}

void test_single_byte_allocations() {
    reset_allocator();
    void *ptrs[20];
    for (int i = 0; i < 20; i++) {
        ptrs[i] = heap_alloc(1);
        if (ptrs[i] == NULL)
            TEST_FAILED();
        *((char *)ptrs[i]) = 'A' + i;
    }

    for (int i = 0; i < 20; i++) {
        if (*((char *)ptrs[i]) != 'A' + i)
            TEST_FAILED();
    }

    for (int i = 0; i < 20; i++) {
        heap_free(ptrs[i]);
    }
    TEST_PASSED();
}

void test_power_of_two_sizes() {
    reset_allocator();
    size_t sizes[] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024};
    void *ptrs[11];

    for (int i = 0; i < 11; i++) {
        ptrs[i] = heap_alloc(sizes[i]);
        if (ptrs[i] == NULL)
            TEST_FAILED();
        memset(ptrs[i], i, sizes[i]);
    }

    for (int i = 0; i < 11; i++) {
        heap_free(ptrs[i]);
    }
    TEST_PASSED();
}

void test_prime_number_sizes() {
    reset_allocator();
    size_t primes[] = {7, 11, 13, 17, 19, 23, 29, 31, 37, 41};
    void *ptrs[10];

    for (int i = 0; i < 10; i++) {
        ptrs[i] = heap_alloc(primes[i]);
        if (ptrs[i] == NULL)
            TEST_FAILED();
    }

    for (int i = 0; i < 10; i++) {
        heap_free(ptrs[i]);
    }
    TEST_PASSED();
}

void test_random_operations_extended() {
    reset_allocator();
    srand(42);

    void *ptrs[100];
    memset(ptrs, 0, sizeof(ptrs));
    int successful_allocs = 0;

    for (int op = 0; op < 500; op++) {
        int idx = rand() % 100;

        if (ptrs[idx] == NULL && rand() % 3 != 0) {
            size_t size = (rand() % 300) + 1;
            ptrs[idx] = heap_alloc(size);
            if (ptrs[idx] != NULL) {
                successful_allocs++;
                memset(ptrs[idx], op & 0xFF, size);
            }
        } else if (ptrs[idx] != NULL) {
            heap_free(ptrs[idx]);
            ptrs[idx] = NULL;
        }

        if (op % 50 == 0 && !check_heap_integrity()) {
            TEST_FAILED();
        }
    }

    for (int i = 0; i < 100; i++) {
        if (ptrs[i] != NULL)
            heap_free(ptrs[i]);
    }

    if (successful_allocs == 0)
        TEST_FAILED();
    TEST_PASSED();
}

void test_allocation_until_full() {
    reset_allocator();
    void *ptrs[1000];
    int count = 0;

    while (count < 1000) {
        ptrs[count] = heap_alloc(100);
        if (ptrs[count] == NULL)
            break;
        count++;
    }

    if (count == 0)
        TEST_FAILED();

    for (int i = 0; i < count; i++) {
        heap_free(ptrs[i]);
    }

    void *test = heap_alloc(100);
    if (test == NULL)
        TEST_FAILED();
    heap_free(test);

    TEST_PASSED();
}

void test_high_frequency_alloc_free() {
    reset_allocator();

    for (int i = 0; i < 1000; i++) {
        void *ptr = heap_alloc(64);
        if (ptr == NULL)
            TEST_FAILED();
        memset(ptr, i & 0xFF, 64);
        heap_free(ptr);
    }

    if (get_alloc_count() != 0)
        TEST_FAILED();
    TEST_PASSED();
}

void test_worst_fit_leaves_larger_fragments() {
    reset_allocator();
    set_allocation_strategy(WORST_FIT);

    void *p1 = heap_alloc(500);
    heap_free(p1);

    void *small = heap_alloc(100);
    if (small == NULL)
        TEST_FAILED();

    if (get_free_block_count() < 1)
        TEST_FAILED();

    heap_free(small);
    TEST_PASSED();
}

void test_data_survives_coalescing() {
    reset_allocator();

    void *p1 = heap_alloc(100);
    void *p2 = heap_alloc(100);
    void *p3 = heap_alloc(100);

    memset(p1, 'A', 100);
    memset(p3, 'C', 100);

    heap_free(p2);

    for (int i = 0; i < 100; i++) {
        if (((char *)p1)[i] != 'A')
            TEST_FAILED();
        if (((char *)p3)[i] != 'C')
            TEST_FAILED();
    }

    heap_free(p1);
    heap_free(p3);
    TEST_PASSED();
}

void test_overlapping_access_detection() {
    reset_allocator();

    void *p1 = heap_alloc(50);
    void *p2 = heap_alloc(50);

    uintptr_t end1 = (uintptr_t)p1 + 50;
    uintptr_t start2 = (uintptr_t)p2;

    if (end1 > start2)
        TEST_FAILED();

    heap_free(p1);
    heap_free(p2);
    TEST_PASSED();
}

void test_alloc_free_alloc_same_size() {
    reset_allocator();

    void *p1 = heap_alloc(256);
    BlockHeader *h1 = (BlockHeader *)((char *)p1 - sizeof(BlockHeader));
    heap_free(p1);

    void *p2 = heap_alloc(256);
    BlockHeader *h2 = (BlockHeader *)((char *)p2 - sizeof(BlockHeader));

    if (h1 != h2)
        TEST_FAILED();

    heap_free(p2);
    TEST_PASSED();
}

void test_allocation_performance() {
    TEST_START();
    reset_allocator();

    const int num_allocs = 1000;
    void *ptrs[num_allocs];
    clock_t start, end;
    double cpu_time_used;

    // Test allocation speed
    start = clock();
    for (int i = 0; i < num_allocs; i++) {
        ptrs[i] = heap_alloc(64);
        if (ptrs[i] == NULL)
            TEST_FAILED();
    }
    end = clock();
    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Time for %d allocations: %f seconds\n", num_allocs, cpu_time_used);

    // Test deallocation speed
    start = clock();
    for (int i = 0; i < num_allocs; i++) {
        heap_free(ptrs[i]);
    }
    end = clock();
    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
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
            void *ptrs[num_allocs];
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
            double time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
            times[strategy][trial] = time_used;
        }
    }

    // Calculate and display average times
    printf("Average performance comparison:\n");
    const char *strategy_names[3] = {"First Fit", "Best Fit", "Worst Fit"};

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

int main(int argc, char *argv[]) {
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
    test_maximum_fragmentation();

    // Error Handling Tests
    test_invalid_free();
    test_double_free();
    test_use_after_free();
    test_heap_integrity();

    // Alocation Strategies Tests
    test_first_fit_strategy();
    test_best_fit_strategy();

    // Block Management Tests
    test_block_splitting();
    test_block_coalescing();
    test_defragmentation();

    // Utility Function Tests
    test_heap_stats();

    // Performance Tests
    test_allocation_performance();
    compare_allocation_strategies();

    printf("\n" ANSI_COLOR_CYAN "=== Enhanced Realloc Tests ===" ANSI_COLOR_RESET "\n");
    test_realloc_null_pointer();
    test_realloc_zero_size();
    test_realloc_exact_same_size();
    test_realloc_with_adjacent_free();
    test_realloc_must_relocate();

    printf("\n" ANSI_COLOR_CYAN "=== Advanced Coalescing Tests ===" ANSI_COLOR_RESET "\n");
    test_coalesce_three_blocks();
    test_coalesce_all_blocks();
    test_coalesce_checkerboard_pattern();

    printf("\n" ANSI_COLOR_CYAN "=== Boundary Tests ===" ANSI_COLOR_RESET "\n");
    test_single_byte_allocations();
    test_power_of_two_sizes();
    test_prime_number_sizes();

    printf("\n" ANSI_COLOR_CYAN "=== Stress Tests ===" ANSI_COLOR_RESET "\n");
    test_random_operations_extended();
    test_allocation_until_full();
    test_high_frequency_alloc_free();

    printf("\n" ANSI_COLOR_CYAN "=== Strategy-Specific Tests ===" ANSI_COLOR_RESET "\n");
    test_worst_fit_leaves_larger_fragments();

    printf("\n" ANSI_COLOR_CYAN "=== Data Integrity Tests ===" ANSI_COLOR_RESET "\n");
    test_data_survives_coalescing();
    test_overlapping_access_detection();

    printf("\n" ANSI_COLOR_CYAN "=== Edge Case Combinations ===" ANSI_COLOR_RESET "\n");
    test_alloc_free_alloc_same_size();

    printf(ANSI_COLOR_MAGENTA "\nAll enhanced tests completed!\n" ANSI_COLOR_RESET);
    return 0;
}
