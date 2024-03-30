#include <stdio.h>
#include <stdlib.h>

int main() {
    // create size of 100 array
    int *p = (int *)malloc(100);
    // free the pointer in the middle of the array
    free(p + 50);
    return 0;
}