#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h> // 添加 time.h 头文件
#include "concurrent_linked_list.h"

#define NUM_THREADS 4
#define OPERATIONS_PER_THREAD 10

list_t list;

void *thread_func(void *arg) {
    int thread_id = *(int *)arg;
    for (int i = 0; i < OPERATIONS_PER_THREAD; i++) {
        int op = rand() % 3; // Randomly choose an operation: 0 for insert, 1 for remove, 2 for print
        int key = rand() % 100; // Random key value between 0 and 99
        if (op == 0) {
            List_Insert(&list, key, thread_id);
        } else if (op == 1) {
            List_Remove(&list, key, thread_id);
        } else {
            List_Print(&list, thread_id);
        }
    }
    return NULL;
}

int main() {
    List_Init(&list);

    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, thread_func, &thread_ids[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    List_Print(&list, -1);

    return 0;
}
