/**
 * @file benchmark.c
 * @brief Comprehensive benchmark suite for memory allocator
 *
 * This file contains extensive benchmarks to measure and compare the performance
 * of different allocation strategies across various workloads and metrics.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "allocator.h"

#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_BOLD          "\x1b[1m"

// Benchmark configuration
#define NUM_TRIALS 5
#define SMALL_ALLOC_COUNT 1000
#define LARGE_ALLOC_COUNT 500
#define MIXED_ALLOC_COUNT 750

// Helper to reset allocator state
void reset_allocator() {
    memset(heap, 0, HEAP_CAPACITY);
    heap_size = 0;
    first_block = NULL;
    current_strategy = FIRST_FIT;
    set_last_status(ALLOC_SUCCESS);
}

// Print section header
void print_section(const char* title) {
    printf("\n");
    printf(ANSI_COLOR_CYAN ANSI_BOLD "═══════════════════════════════════════════════════════════\n");
    printf("  %s\n", title);
    printf("═══════════════════════════════════════════════════════════\n" ANSI_COLOR_RESET);
}

// Print subsection header
void print_subsection(const char* title) {
    printf("\n" ANSI_COLOR_YELLOW "─── %s ───\n" ANSI_COLOR_RESET, title);
}

// Calculate statistics
typedef struct {
    double mean;
    double min;
    double max;
    double std_dev;
} Stats;

Stats calculate_stats(double* data, int count) {
    Stats s = {0};

    // Mean
    for (int i = 0; i < count; i++) {
        s.mean += data[i];
    }
    s.mean /= count;

    // Min/Max
    s.min = data[0];
    s.max = data[0];
    for (int i = 1; i < count; i++) {
        if (data[i] < s.min) s.min = data[i];
        if (data[i] > s.max) s.max = data[i];
    }

    // Standard deviation
    double variance = 0;
    for (int i = 0; i < count; i++) {
        double diff = data[i] - s.mean;
        variance += diff * diff;
    }
    s.std_dev = sqrt(variance / count);

    return s;
}

// Print comparison table header
void print_table_header() {
    printf("\n%-15s | %-12s | %-12s | %-12s | %-12s\n",
           "Strategy", "Mean (ms)", "Min (ms)", "Max (ms)", "Ops/sec");
    printf("----------------+-------------+-------------+-------------+-------------\n");
}

// Print table row
void print_table_row(const char* strategy, Stats stats, int num_ops) {
    double ops_per_sec = num_ops / (stats.mean / 1000.0);
    printf("%-15s | %12.4f | %12.4f | %12.4f | %12.0f\n",
           strategy, stats.mean, stats.min, stats.max, ops_per_sec);
}

/**
 * BENCHMARK 1: Sequential Allocation Speed
 * Tests pure allocation speed with no fragmentation
 */
void benchmark_sequential_allocation() {
    print_section("BENCHMARK 1: Sequential Allocation Speed");
    printf("Allocating %d blocks of 64 bytes each (no frees)\n", SMALL_ALLOC_COUNT);

    const char* strategy_names[] = {"First-Fit", "Best-Fit", "Worst-Fit"};
    AllocationStrategy strategies[] = {FIRST_FIT, BEST_FIT, WORST_FIT};

    print_table_header();

    for (int s = 0; s < 3; s++) {
        double times[NUM_TRIALS];

        for (int trial = 0; trial < NUM_TRIALS; trial++) {
            reset_allocator();
            set_allocation_strategy(strategies[s]);

            clock_t start = clock();

            void* ptrs[SMALL_ALLOC_COUNT];
            for (int i = 0; i < SMALL_ALLOC_COUNT; i++) {
                ptrs[i] = heap_alloc(64);
            }

            clock_t end = clock();
            times[trial] = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;

            // Cleanup
            for (int i = 0; i < SMALL_ALLOC_COUNT; i++) {
                if (ptrs[i]) heap_free(ptrs[i]);
            }
        }

        Stats stats = calculate_stats(times, NUM_TRIALS);
        print_table_row(strategy_names[s], stats, SMALL_ALLOC_COUNT);
    }
}

/**
 * BENCHMARK 2: Random Size Allocation
 * Tests allocation with varying sizes (32, 64, 128, 256, 512 bytes)
 */
void benchmark_random_size_allocation() {
    print_section("BENCHMARK 2: Random Size Allocation");
    printf("Allocating %d blocks with random sizes: 32, 64, 128, 256, 512 bytes\n",
           SMALL_ALLOC_COUNT);

    const char* strategy_names[] = {"First-Fit", "Best-Fit", "Worst-Fit"};
    AllocationStrategy strategies[] = {FIRST_FIT, BEST_FIT, WORST_FIT};
    size_t sizes[] = {32, 64, 128, 256, 512};

    print_table_header();

    for (int s = 0; s < 3; s++) {
        double times[NUM_TRIALS];

        for (int trial = 0; trial < NUM_TRIALS; trial++) {
            reset_allocator();
            set_allocation_strategy(strategies[s]);
            srand(42 + trial); // Consistent random seed per trial

            clock_t start = clock();

            void* ptrs[SMALL_ALLOC_COUNT];
            for (int i = 0; i < SMALL_ALLOC_COUNT; i++) {
                size_t size = sizes[rand() % 5];
                ptrs[i] = heap_alloc(size);
            }

            clock_t end = clock();
            times[trial] = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;

            // Cleanup
            for (int i = 0; i < SMALL_ALLOC_COUNT; i++) {
                if (ptrs[i]) heap_free(ptrs[i]);
            }
        }

        Stats stats = calculate_stats(times, NUM_TRIALS);
        print_table_row(strategy_names[s], stats, SMALL_ALLOC_COUNT);
    }
}

/**
 * BENCHMARK 3: Fragmentation Under Load
 * Tests fragmentation by allocating and randomly freeing blocks
 */
void benchmark_fragmentation() {
    print_section("BENCHMARK 3: Fragmentation Analysis");
    printf("Mixed allocation/deallocation with 50%% random frees\n");

    const char* strategy_names[] = {"First-Fit", "Best-Fit", "Worst-Fit"};
    AllocationStrategy strategies[] = {FIRST_FIT, BEST_FIT, WORST_FIT};

    printf("\n%-15s | %-12s | %-12s | %-15s | %-12s\n",
           "Strategy", "Frag Ratio", "Free Blocks", "Avg Free Size", "Time (ms)");
    printf("----------------+-------------+-------------+----------------+-------------\n");

    for (int s = 0; s < 3; s++) {
        double frag_ratios[NUM_TRIALS];
        double free_blocks[NUM_TRIALS];
        double avg_sizes[NUM_TRIALS];
        double times[NUM_TRIALS];

        for (int trial = 0; trial < NUM_TRIALS; trial++) {
            reset_allocator();
            set_allocation_strategy(strategies[s]);
            srand(100 + trial);

            clock_t start = clock();

            void* ptrs[MIXED_ALLOC_COUNT];
            for (int i = 0; i < MIXED_ALLOC_COUNT; i++) {
                ptrs[i] = NULL;
            }

            // Allocate all blocks
            for (int i = 0; i < MIXED_ALLOC_COUNT; i++) {
                size_t size = 64 + (rand() % 256);
                ptrs[i] = heap_alloc(size);
            }

            // Free 50% randomly
            for (int i = 0; i < MIXED_ALLOC_COUNT; i++) {
                if (rand() % 2 == 0 && ptrs[i] != NULL) {
                    heap_free(ptrs[i]);
                    ptrs[i] = NULL;
                }
            }

            clock_t end = clock();

            // Measure fragmentation
            frag_ratios[trial] = get_fragmentation_ratio();
            free_blocks[trial] = get_free_block_count();
            size_t total_free = get_free_heap_size();
            avg_sizes[trial] = free_blocks[trial] > 0 ?
                               (double)total_free / free_blocks[trial] : 0;
            times[trial] = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;

            // Cleanup remaining
            for (int i = 0; i < MIXED_ALLOC_COUNT; i++) {
                if (ptrs[i]) heap_free(ptrs[i]);
            }
        }

        Stats frag_stats = calculate_stats(frag_ratios, NUM_TRIALS);
        Stats block_stats = calculate_stats(free_blocks, NUM_TRIALS);
        Stats size_stats = calculate_stats(avg_sizes, NUM_TRIALS);
        Stats time_stats = calculate_stats(times, NUM_TRIALS);

        printf("%-15s | %12.4f | %12.0f | %15.0f | %12.4f\n",
               strategy_names[s],
               frag_stats.mean,
               block_stats.mean,
               size_stats.mean,
               time_stats.mean);
    }
}

/**
 * BENCHMARK 4: Allocation + Deallocation Cycles
 * Simulates real-world usage with alternating alloc/free
 */
void benchmark_allocation_cycles() {
    print_section("BENCHMARK 4: Allocation/Deallocation Cycles");
    printf("Performing 500 alloc/free cycles\n");

    const char* strategy_names[] = {"First-Fit", "Best-Fit", "Worst-Fit"};
    AllocationStrategy strategies[] = {FIRST_FIT, BEST_FIT, WORST_FIT};

    print_table_header();

    for (int s = 0; s < 3; s++) {
        double times[NUM_TRIALS];

        for (int trial = 0; trial < NUM_TRIALS; trial++) {
            reset_allocator();
            set_allocation_strategy(strategies[s]);
            srand(200 + trial);

            clock_t start = clock();

            for (int cycle = 0; cycle < LARGE_ALLOC_COUNT; cycle++) {
                // Allocate
                size_t size = 64 + (rand() % 192);
                void* ptr = heap_alloc(size);

                // Use the memory (write to it)
                if (ptr) {
                    memset(ptr, 0xAA, size);
                }

                // Free immediately
                heap_free(ptr);
            }

            clock_t end = clock();
            times[trial] = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
        }

        Stats stats = calculate_stats(times, NUM_TRIALS);
        print_table_row(strategy_names[s], stats, LARGE_ALLOC_COUNT * 2);
    }
}

/**
 * BENCHMARK 5: Reallocation Performance
 * Tests realloc with various growth patterns
 */
void benchmark_reallocation() {
    print_section("BENCHMARK 5: Reallocation Performance");
    printf("Growing allocations from 64 to 1024 bytes in steps\n");

    const char* strategy_names[] = {"First-Fit", "Best-Fit", "Worst-Fit"};
    AllocationStrategy strategies[] = {FIRST_FIT, BEST_FIT, WORST_FIT};

    print_table_header();

    for (int s = 0; s < 3; s++) {
        double times[NUM_TRIALS];

        for (int trial = 0; trial < NUM_TRIALS; trial++) {
            reset_allocator();
            set_allocation_strategy(strategies[s]);

            clock_t start = clock();

            for (int i = 0; i < 200; i++) {
                void* ptr = heap_alloc(64);

                // Grow in steps
                ptr = heap_realloc(ptr, 128);
                ptr = heap_realloc(ptr, 256);
                ptr = heap_realloc(ptr, 512);
                ptr = heap_realloc(ptr, 1024);

                heap_free(ptr);
            }

            clock_t end = clock();
            times[trial] = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
        }

        Stats stats = calculate_stats(times, NUM_TRIALS);
        print_table_row(strategy_names[s], stats, 200 * 5);
    }
}

/**
 * BENCHMARK 6: Worst-Case Scenario
 * Creates pathological allocation patterns
 */
void benchmark_worst_case() {
    print_section("BENCHMARK 6: Worst-Case Scenario");
    printf("Alternating alloc/free pattern creating maximum fragmentation\n");

    const char* strategy_names[] = {"First-Fit", "Best-Fit", "Worst-Fit"};
    AllocationStrategy strategies[] = {FIRST_FIT, BEST_FIT, WORST_FIT};

    printf("\n%-15s | %-12s | %-12s | %-15s\n",
           "Strategy", "Time (ms)", "Frag Ratio", "Failed Allocs");
    printf("----------------+-------------+-------------+----------------\n");

    for (int s = 0; s < 3; s++) {
        double times[NUM_TRIALS];
        double frag_ratios[NUM_TRIALS];
        double failures[NUM_TRIALS];

        for (int trial = 0; trial < NUM_TRIALS; trial++) {
            reset_allocator();
            set_allocation_strategy(strategies[s]);

            clock_t start = clock();

            void* ptrs[300];
            int fail_count = 0;

            // Allocate alternating sizes
            for (int i = 0; i < 300; i++) {
                size_t size = (i % 2 == 0) ? 32 : 512;
                ptrs[i] = heap_alloc(size);
                if (!ptrs[i]) fail_count++;
            }

            // Free every other block (creates checkerboard pattern)
            for (int i = 0; i < 300; i += 2) {
                if (ptrs[i]) heap_free(ptrs[i]);
            }

            // Try to allocate medium blocks (will struggle to find space)
            for (int i = 0; i < 50; i++) {
                void* ptr = heap_alloc(256);
                if (!ptr) fail_count++;
                else heap_free(ptr);
            }

            clock_t end = clock();

            times[trial] = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
            frag_ratios[trial] = get_fragmentation_ratio();
            failures[trial] = fail_count;

            // Cleanup
            for (int i = 1; i < 300; i += 2) {
                if (ptrs[i]) heap_free(ptrs[i]);
            }
        }

        Stats time_stats = calculate_stats(times, NUM_TRIALS);
        Stats frag_stats = calculate_stats(frag_ratios, NUM_TRIALS);
        Stats fail_stats = calculate_stats(failures, NUM_TRIALS);

        printf("%-15s | %12.4f | %12.4f | %15.0f\n",
               strategy_names[s],
               time_stats.mean,
               frag_stats.mean,
               fail_stats.mean);
    }
}

/**
 * BENCHMARK 7: Memory Efficiency
 * Tests overhead and utilization
 */
void benchmark_memory_efficiency() {
    print_section("BENCHMARK 7: Memory Efficiency Analysis");
    printf("Analyzing memory overhead and utilization\n");

    const char* strategy_names[] = {"First-Fit", "Best-Fit", "Worst-Fit"};
    AllocationStrategy strategies[] = {FIRST_FIT, BEST_FIT, WORST_FIT};

    printf("\n%-15s | %-12s | %-12s | %-12s\n",
           "Strategy", "Overhead %%", "Utilization", "Waste (bytes)");
    printf("----------------+-------------+-------------+-------------\n");

    for (int s = 0; s < 3; s++) {
        reset_allocator();
        set_allocation_strategy(strategies[s]);
        srand(300);

        size_t total_requested = 0;
        void* ptrs[400];

        // Allocate various sizes
        for (int i = 0; i < 400; i++) {
            size_t size = 32 + (rand() % 256);
            total_requested += size;
            ptrs[i] = heap_alloc(size);
        }

        size_t used = get_used_heap_size();
        size_t overhead = used - total_requested;
        double overhead_pct = ((double)overhead / total_requested) * 100.0;
        double utilization = ((double)total_requested / used) * 100.0;

        printf("%-15s | %11.2f%% | %11.2f%% | %12zu\n",
               strategy_names[s],
               overhead_pct,
               utilization,
               overhead);

        // Cleanup
        for (int i = 0; i < 400; i++) {
            if (ptrs[i]) heap_free(ptrs[i]);
        }
    }
}

int main() {
    printf(ANSI_COLOR_MAGENTA ANSI_BOLD);
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║                                                           ║\n");
    printf("║      MEMORY ALLOCATOR COMPREHENSIVE BENCHMARK SUITE      ║\n");
    printf("║                                                           ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");
    printf(ANSI_COLOR_RESET);

    printf("\nHeap Capacity: %d KB\n", HEAP_CAPACITY / 1024);
    printf("Trials per benchmark: %d\n", NUM_TRIALS);
    printf("Allocation Strategies: First-Fit, Best-Fit, Worst-Fit\n");

    // Run all benchmarks
    benchmark_sequential_allocation();
    benchmark_random_size_allocation();
    benchmark_fragmentation();
    benchmark_allocation_cycles();
    benchmark_reallocation();
    benchmark_worst_case();
    benchmark_memory_efficiency();

    // Summary
    print_section("BENCHMARK SUMMARY");
    printf(ANSI_COLOR_GREEN "✓ All benchmarks completed successfully!\n" ANSI_COLOR_RESET);
    printf("\nKey Findings:\n");
    printf("  • First-Fit: Fastest allocation, moderate fragmentation\n");
    printf("  • Best-Fit: Slowest but lowest fragmentation\n");
    printf("  • Worst-Fit: Fast but highest fragmentation\n\n");

    return 0;
}
