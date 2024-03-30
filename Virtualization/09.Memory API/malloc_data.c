#include <stdio.h>
#include <stdlib.h>

int main() {
    // create size of 100 array
    int *p = (int *)malloc(100);
    // set p[100] = 0
    p[100] = 0;
    return 0;
}