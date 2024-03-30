#include <stdio.h>
#include <stdlib.h>

int main() {
    // create size of 100 array
    int *p = (int *)malloc(100);
    free(p);
    // use p
    p[0] = 0;
    return 0;
}