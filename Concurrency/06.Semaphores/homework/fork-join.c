#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include "zemaphore.h"
#include "common_threads.h"

Zem_t s; 

void *child(void *arg) {
    sleep(1);
    printf("child\n");
    // use semaphore here
    Zem_post(&s);
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t p;
    printf("parent: begin\n");
    // init semaphore here
    Zem_init(&s, 0);
    Pthread_create(&p, NULL, child, NULL);
    // use semaphore here
    Zem_wait(&s);
    printf("parent: end\n");
    return 0;
}

