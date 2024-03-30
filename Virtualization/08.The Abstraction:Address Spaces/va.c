#include <stdio.h>
#include <stdlib.h>

int global = 1;

int main(int argc, char *argv[]) {
    // Declaring a variable x on the stack
    int x = 3;

    // Printing the location of the code
    printf("location of code : %p\n", (void *) main);
    // Printing the location of global variable
    printf("location of data : %p\n", (void *) &global);
    // Printing the location of heap memory
    printf("location of heap : %p\n", (void *) malloc(1));
    // Printing the location of stack variable x
    printf("location of stack : %p\n", (void *) &x);
    
    return x;
}