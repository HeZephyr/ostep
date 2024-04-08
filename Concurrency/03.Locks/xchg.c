#include <stdio.h>

unsigned int global = 0;

static inline unsigned int xchg(volatile unsigned int *addr, unsigned int newval) {
    unsigned int result
    ;
    // The + in "+m" denotes a read-modify-write operand.
    // It means that the old value is read and the new value is written.
    __asm__ __volatile__("lock; xchgl %0, %1"
                         : "+m"(*addr), "=a"(result)
                         : "1"(newval)
                         : "cc");
    return result;
}
int main(int argc, char *argv[]) {

    // Before successful xchg
    printf("before successful xchg: %d\n", global);
    // Perform successful xchg operation
    int result = xchg(&global, 300);
    // Print result after successful xchg
    printf("after successful xchg: %d (old: %d)\n", global, result);


    return 0;
}