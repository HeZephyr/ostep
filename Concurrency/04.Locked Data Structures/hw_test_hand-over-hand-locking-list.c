#include "hand-over-hand-locking-list.c"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h> // For gettimeofday()

#define NUM_THREADS 4
#define OPERATIONS_PER_THREAD 100000

list_t list;

void *thread_func(void *arg) {
    int thread_id = *(int *)arg;
    srand(time(NULL) + thread_id);
    for (int i = 0; i < OPERATIONS_PER_THREAD; i++) {
        int op = rand() % 2; // Randomly choose an operation: 0 for insert, 1 for remove
        int key = rand() % 1000; // Random key value between 0 and 999
        if (op == 0) {
            List_Insert(&list, key);
        } else {
            List_Remove(&list, key);
        }
    }
    return NULL;
}

int main() {
    List_Init(&list);

    struct timeval start, end;
    gettimeofday(&start, NULL);

    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, thread_func, &thread_ids[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    gettimeofday(&end, NULL);
    double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    printf("Elapsed time: %.6f seconds\n", elapsed_time);

    List_Free(&list);

    return 0;
}
