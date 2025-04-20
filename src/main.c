#include <stdio.h>
#include "allocator.h"

int main() {
    int* root1 = heap_alloc(26 * sizeof(int));
    int* root2 = heap_alloc(15 * sizeof(int));
    int* root3 = heap_alloc(5 * sizeof(int));
    for (int i = 0; i < 26; ++i) {
        root1[i] = i;
    }

    for (int i = 0; i < 26; ++i) {
        printf("root1[%d] = %d\n", i, root1[i]);
    }

    print_heap();

    heap_free(root3);

    print_heap();

    heap_free(root2);

    print_heap();

    heap_free(root1);

    print_heap();

    return 0;
}
