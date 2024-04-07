#include <stdio.h>

#include "common_threads.h"

int balance = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
void* worker(void* arg) {
    pthread_mutex_lock(&mutex);
    balance++; // unprotected access
    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t p;
    Pthread_create(&p, NULL, worker, NULL);
    pthread_mutex_lock(&mutex);
    balance++; // unprotected access
    pthread_mutex_unlock(&mutex);
    Pthread_join(p, NULL);
    return 0;
}