#include <stdio.h>
#include <string.h>
#include "allocator.h"

int main() {
    int* root = heap_alloc(4 * sizeof(int));
    print_heap();
    return 0;
}
