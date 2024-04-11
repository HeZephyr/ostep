#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common_threads.h"
#include "zemaphore.h"

#define NUM_SMOKERS 3

/*
* Combination 1: Tobacco + Match
* Combination 2: Tobacco + Paper
* Combination 3: Match + Paper
*/
Zem_t tobacco_paper;
Zem_t tobacco_match;
Zem_t paper_match;
Zem_t finish;
void smoker_1() {
    while(1) {
        Zem_wait(&tobacco_paper);
        printf("Smoker 1 is smoking\n");
        Zem_post(&finish);  // inform agent that smoking is done
    }
}
void smoker_2() {
    while(1) {
        Zem_wait(&tobacco_match);
        printf("Smoker 2 is smoking\n");
        Zem_post(&finish);
    }
}
void smoker_3() {
    while(1) {
        Zem_wait(&paper_match);
        printf("Smoker 3 is smoking\n");
        Zem_post(&finish);
    }
}
void agent() {
    while(1) {
        int r = rand() % NUM_SMOKERS;
        switch(r) {
            case 0:
                Zem_post(&tobacco_paper);
                break;
            case 1:
                Zem_post(&tobacco_match);
                break;
            case 2:
                Zem_post(&paper_match);
                break;
        }
        Zem_wait(&finish);
        sleep(1);
    }

}
int main() {
    pthread_t tids[NUM_SMOKERS + 1];
    Zem_init(&tobacco_paper, 0);
    Zem_init(&tobacco_match, 0);
    Zem_init(&paper_match, 0);
    Zem_init(&finish, 0);
    Pthread_create(&tids[0], NULL, (void *)agent, NULL);
    Pthread_create(&tids[1], NULL, (void *)smoker_1, NULL);
    Pthread_create(&tids[2], NULL, (void *)smoker_2, NULL);
    Pthread_create(&tids[3], NULL, (void *)smoker_3, NULL);
    for (int i = 0; i < NUM_SMOKERS + 1; i++) {
        Pthread_join(tids[i], NULL);
    }
    return 0;
}