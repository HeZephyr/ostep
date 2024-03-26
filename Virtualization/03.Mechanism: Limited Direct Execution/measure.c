#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#define ITERATIONS 1000

// Measure the cost of a system call
void measure_system_call() {
    struct timeval start, end;
    gettimeofday(&start, NULL);
    for (int i = 0; i < ITERATIONS; i++) {
        read(0, NULL, 0);  // Example of a simple system call
    }
    gettimeofday(&end, NULL);
    double time_per_call = ((double)(end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec)) / ITERATIONS;
    printf("Estimated cost of a system call: %f microseconds\n", time_per_call);
}

// Measure the precision of gettimeofday()
void measure_gettimeofday_precision() {
    struct timeval start, end;
    gettimeofday(&start, NULL);
    for (int i = 0; i < ITERATIONS; i++) {
        gettimeofday(&end, NULL);
    }
    double precision = ((double)(end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec)) / ITERATIONS;
    printf("Precision of gettimeofday(): %f microseconds\n", precision);
}

// Measure the cost of a context switch
void measure_context_switch() {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Child process
        close(pipefd[1]);
        struct timeval start, end;
        gettimeofday(&start, NULL);
        read(pipefd[0], NULL, 0);
        gettimeofday(&end, NULL);
        double time_diff = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
        printf("Child process context switch time: %f microseconds\n", time_diff);
        exit(EXIT_SUCCESS);
    } else {
        // Parent process
        close(pipefd[0]);
        struct timeval start, end;
        gettimeofday(&start, NULL);
        write(pipefd[1], "", 1);
        wait(NULL);
        gettimeofday(&end, NULL);
        double time_diff = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
        printf("Parent process context switch time: %f microseconds\n", time_diff);
    }
}

int main() {
    measure_system_call();
    measure_gettimeofday_precision();
    measure_context_switch();
    return 0;
}
