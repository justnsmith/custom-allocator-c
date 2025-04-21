#include <stdio.h>
#include <string.h>
#include "allocator.h"

int main() {
    // Example of using the custom allocator

    int *arr = heap_alloc(sizeof(int) * 5);  // Example allocation
    if (arr == NULL) {
        printf("Memory allocation failed!\n");
        return 1;
    }

    // Do something with the allocated memory
    for (int i = 0; i < 5; i++) {
        arr[i] = i * 2;
    }

    // Print out values
    for (int i = 0; i < 5; i++) {
        printf("arr[%d] = %d\n", i, arr[i]);
    }

    // Print current heap
    print_heap();

    // Free the allocated memory
    heap_free(arr);

    return 0;
}
