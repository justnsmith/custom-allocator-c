#include <stdio.h>
#include <string.h>
#include "allocator.h"

int main() {
    int* root = heap_alloc(4 * sizeof(int));
    int* root2 = heap_alloc(7 * sizeof(int));
    heap_free(root2);
    heap_free(root);
    print_heap();
    return 0;
}
