#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include "concurrent_hashmap.h"

#define NUM_THREADS 10
#define NUM_OPERATIONS 100000

void* perform_operations(void* arg) {
    hashmap_t* hashmap = (hashmap_t*)arg;
    for (int i = 0; i < NUM_OPERATIONS; ++i) {
        char key[10];
        char value[10];
        sprintf(key, "key%d", rand() % NUM_OPERATIONS);
        sprintf(value, "value%d", rand() % NUM_OPERATIONS);
        put(hashmap, key, value);
        get(hashmap, key);
    }
    pthread_exit(NULL);
}

int main() {
    struct timeval start, end;
    pthread_t threads[NUM_THREADS];
    hashmap_t* hashmap = create_hashmap();

    gettimeofday(&start, NULL);
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_create(&threads[i], NULL, perform_operations, (void*)hashmap);
    }
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], NULL);
    }
    gettimeofday(&end, NULL);

    long long elapsed = (end.tv_sec - start.tv_sec) * 1000000LL + (end.tv_usec - start.tv_usec);
    printf("Time taken: %lld microseconds\n", elapsed);

    destroy_hashmap(hashmap);
    return 0;
}
