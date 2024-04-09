#include <stdio.h>
#include <pthread.h>

#define NUM_THREADS 4 // Number of threads for testing

typedef struct __counter_t {
    int global; // global count
    pthread_mutex_t glock; // global lock
    int local[NUM_THREADS]; // local count (per thread)
    pthread_mutex_t llock[NUM_THREADS]; // ... and locks
    int threshold; // update frequency
} counter_t;

/*
 * Initialize the counter data structure.
    * @param c: The counter data structure to initialize.
    * @param threshold: The threshold value for the counter.
*/
void init(counter_t *c, int threshold) {
    c->threshold = threshold;
    c->global = 0;
    pthread_mutex_init(&c->glock, NULL);
    for (int i = 0; i < NUM_THREADS; i++) {
        c->local[i] = 0;
        pthread_mutex_init(&c->llock[i], NULL);
    }
}

void update(counter_t *c, int threadID, int amt) {
    int cpu = threadID % NUM_THREADS;
    pthread_mutex_lock(&c->llock[cpu]);
    c->local[cpu] += amt; // assumes amt > 0
    if (c->local[cpu] >= c->threshold) { // transfer to global
        pthread_mutex_lock(&c->glock);
        c->global += c->local[cpu];
        pthread_mutex_unlock(&c->glock);
        c->local[cpu] = 0;
    }
    pthread_mutex_unlock(&c->llock[cpu]);
}

int get(counter_t *c) {
    pthread_mutex_lock(&c->glock);
    int val = c->global;
    pthread_mutex_unlock(&c->glock);
    return val; // only approximate!
}

void *thread_func(void *arg) {
    counter_t *counter = (counter_t *)arg;
    unsigned long tid = (unsigned long)pthread_self();
    int threadID = (int)tid;
    for (int i = 0; i < 100000; i++) {
        update(counter, threadID, 1); // Increment the counter
    }
    return NULL;
}

int main() {
    counter_t counter;
    init(&counter, 1000); // Set threshold to 1000

    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, thread_func, (void *)&counter);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Final count: %d\n", get(&counter));

    return 0;
}
