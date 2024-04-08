#include <stdio.h>

int global = 0;

// Function to perform a compare and swap operation
// Parameters:
//   ptr: pointer to the memory location where the operation will be performed
//   old: expected value to compare against
//   new: new value to store if the comparison succeeds
// Returns:
//   1 if the comparison succeeded and the value was updated, 0 otherwise
char compare_and_swap(int *ptr, int old, int new) {
    unsigned char ret;
    // Assembly code to perform the compare and swap operation
    // Note: sete sets a byte not the word
    __asm__ __volatile__ (
        " lock\n"   // 指令前缀：锁定下一个ins（%1）访问的缓存行。简单地说，ASM看起来像：lock cmpxchgl %2,%1
        " cmpxchgl %2,%1\n"
        " sete %0\n"    // Set byte if equal (ZF=1).
        : "=q" (ret), "=m" (*ptr)   //outputs
        : "r" (new), "m" (*ptr), "a" (old) // inputs
        : "memory");    // labels
    return ret;
}


int main(int argc, char *argv[]) {
    // Before successful compare and swap
    printf("before successful cas: %d\n", global);
    // Perform successful compare and swap operation
    int success = compare_and_swap(&global, 0, 100);
    // Print result after successful compare and swap
    printf("after successful cas: %d (success: %d)\n", global, success);
    
    // Before failing compare and swap
    printf("before failing cas: %d\n", global);
    // Perform failing compare and swap operation
    success = compare_and_swap(&global, 0, 200);
    // Print result after failing compare and swap
    printf("after failing cas: %d (old: %d)\n", global, success);

    return 0;
}