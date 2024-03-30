#include <stdio.h>
#include <stdlib.h>

void malloc_but_nofree() {
    int *p = (int *)malloc(sizeof(int));
}
int main() {
    malloc_but_nofree();
    return 0;
}