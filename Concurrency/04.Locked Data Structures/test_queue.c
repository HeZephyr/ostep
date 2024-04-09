#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h> // 添加 time.h 头文件
#include "concurrent_queue.h"

#define NUM_THREADS 4
#define OPERATIONS_PER_THREAD 10

queue_t queue;

void *thread_func(void *arg) {
    int thread_id = *(int *)arg;
    srand(time(NULL) + thread_id);
    for (int i = 0; i < OPERATIONS_PER_THREAD; i++) {
        int op = rand() % 2; // Randomly choose an operation: 0 for enqueue, 1 for dequeue
        if (op == 0) {
            int value = rand() % 100; // Random value between 0 and 99
            printf("Thread %d enqueued value: %d\n", thread_id, value);
            Queue_Enqueue(&queue, value);
        } else {
            int value;
            if (Queue_Dequeue(&queue, &value) == 0) {
                printf("Thread %d dequeued value: %d\n", thread_id, value);
            }
        }
    }
    return NULL;
}

int main() {
    Queue_Init(&queue);

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
