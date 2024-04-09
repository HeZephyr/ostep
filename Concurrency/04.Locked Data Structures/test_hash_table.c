#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "concurrent_hash_table.h"

#define NUM_THREADS 4
#define OPERATIONS_PER_THREAD 10

hash_t hash_table;

void *thread_func(void *arg) {
    int thread_id = *(int *)arg;
    srand(time(NULL) + thread_id);
    for (int i = 0; i < OPERATIONS_PER_THREAD; i++) {
        int op = rand() % 2; // Randomly choose an operation: 0 for insert, 1 for remove
        int key = rand() % 100; // Random key value between 0 and 99
        if (op == 0) {
            printf("Thread %d: Inserting key %d\n", thread_id, key);
            Hash_Insert(&hash_table, key, thread_id);
        } else {
            printf("Thread %d: Looking up key %d\n", thread_id, key);
            Hash_Remove(&hash_table, key, thread_id);
        }
    }
    return NULL;
}

int main() {
    Hash_Init(&hash_table);

    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, thread_func, &thread_ids[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
