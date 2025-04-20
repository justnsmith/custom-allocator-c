#include <stdio.h>
#include <string.h>
#include "allocator.h"

void test_realloc() {
    printf("=== TEST: realloc(NULL, size) ===\n");
    int* p1 = heap_realloc(NULL, 10 * sizeof(int));
    for (int i = 0; i < 10; ++i) p1[i] = i * 2;
    for (int i = 0; i < 10; ++i) printf("p1[%d] = %d\n", i, p1[i]);
    print_heap();

    printf("\n=== TEST: realloc(ptr, 0) ===\n");
    void* p2 = heap_realloc(p1, 0); // Should free p1
    printf("Returned from realloc(ptr, 0): %p\n", p2);
    print_heap();

    printf("\n=== TEST: shrink block ===\n");
    int* p3 = heap_alloc(20 * sizeof(int));
    for (int i = 0; i < 20; ++i) p3[i] = i + 100;
    p3 = heap_realloc(p3, 10 * sizeof(int)); // Should split
    for (int i = 0; i < 10; ++i) printf("p3[%d] = %d\n", i, p3[i]);
    print_heap();

    printf("\n=== TEST: expand into next free block ===\n");
    int* p4 = heap_alloc(5 * sizeof(int));
    heap_free(p4); // Free next block so we can extend into it
    p3 = heap_realloc(p3, 25 * sizeof(int)); // Should merge and split
    for (int i = 0; i < 10; ++i) printf("p3[%d] = %d\n", i, p3[i]);
    print_heap();

    printf("\n=== TEST: expand into new block ===\n");
    int* p5 = heap_alloc(8 * sizeof(int));
    for (int i = 0; i < 8; ++i) p5[i] = i + 200;
    int* p6 = heap_realloc(p5, 40 * sizeof(int)); // Should allocate new block
    for (int i = 0; i < 8; ++i) printf("p6[%d] = %d\n", i, p6[i]);
    print_heap();

    // Clean up
    heap_free(p3);
    heap_free(p6);
}

int main() {
    test_realloc();
    return 0;
}
