#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common_threads.h"
#include "zemaphore.h"

// If done correctly, each child should print their "before" message
// before either prints their "after" message. Test by adding sleep(1)
// calls in various locations.

// You likely need two semaphores to do this correctly, and some
// other integers to track things.

typedef struct __barrier_t {
    // add semaphores and other information here
    Zem_t mutex;
    Zem_t barrier;
    int num_threads;
    int num_arrived;
} barrier_t;


// the single barrier we are using for this program
barrier_t b;

void barrier_init(barrier_t *b, int num_threads) {
    // initialization code goes here
    Zem_init(&b->mutex, 1);
    Zem_init(&b->barrier, 0);
    b->num_threads = num_threads;
    b->num_arrived = 0;
}

void barrier(barrier_t *b) {
    // barrier code goes here
    Zem_wait(&b->mutex); // enter critical section
    b->num_arrived++;
    Zem_post(&b->mutex); // leave critical section

    if (b->num_arrived == b->num_threads) {
        for (int i = 0; i < b->num_threads; i++) {
            Zem_post(&b->barrier);
        }
    }
    Zem_wait(&b->barrier);
}

//
// XXX: don't change below here (just run it!)
//
typedef struct __tinfo_t {
    int thread_id;
} tinfo_t;

void *child(void *arg) {
    sleep(1);
    tinfo_t *t = (tinfo_t *) arg;
    printf("child %d: before\n", t->thread_id);
    barrier(&b);
    printf("child %d: after\n", t->thread_id);
    return NULL;
}


// run with a single argument indicating the number of 
// threads you wish to create (1 or more)
int main(int argc, char *argv[]) {
    assert(argc == 2);
    int num_threads = atoi(argv[1]);
    assert(num_threads > 0);

    pthread_t p[num_threads];
    tinfo_t t[num_threads];

    printf("parent: begin\n");
    barrier_init(&b, num_threads);
    
    int i;
    for (i = 0; i < num_threads; i++) {
        t[i].thread_id = i;
        Pthread_create(&p[i], NULL, child, &t[i]);
    }

    for (i = 0; i < num_threads; i++) 
	    Pthread_join(p[i], NULL);

    printf("parent: end\n");
    return 0;
}

