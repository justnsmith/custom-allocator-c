#include <stdio.h>
#include "allocator.h"

int main() {
    int* root = heap_alloc(26 * sizeof(int));
    for (int i = 0; i < 26; ++i) {
        root[i] = i;
    }

    for (int i = 0; i < 26; ++i) {
        printf("root[%d] = %d\n", i, root[i]);
    }

    heap_free(root);
    root = NULL;

    return 0;
}
