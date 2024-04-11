#include "common_threads.h"
#include "zemaphore.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h> // free, malloc

// Little Book of Semaphores: chapter 4.5
Zem_t tobacco_sem, paper_sem, match_sem, agent_sem, lock;
bool has_tobacco = false;
bool has_paper = false;
bool has_match = false;

void init_sem() {
  Zem_init(&tobacco_sem, 0);
  Zem_init(&paper_sem, 0);
  Zem_init(&match_sem, 0);
  Zem_init(&agent_sem, 1);
  Zem_init(&lock, 1);
}

void tobacco_pusher() {
  if (has_paper) {
    has_paper = false;
    Zem_post(&match_sem);
  } else if (has_match) {
    has_match = false;
    Zem_post(&paper_sem);
  } else
    has_tobacco = true;
}

void paper_pusher() {
  if (has_match) {
    has_match = false;
    Zem_post(&tobacco_sem);
  } else if (has_tobacco) {
    has_tobacco = false;
    Zem_post(&match_sem);
  } else
    has_paper = true;
}

void match_pusher() {
  if (has_paper) {
    has_paper = false;
    Zem_post(&tobacco_sem);
  } else if (has_tobacco) {
    has_tobacco = false;
    Zem_post(&paper_sem);
  } else
    has_match = true;
}

void *agent(void *arg) {
  int type = *(int *)arg;
  Zem_wait(&agent_sem);
  Zem_wait(&lock);
  switch (type) {
  case 0:
    tobacco_pusher();
    paper_pusher();
    break;
  case 1:
    paper_pusher();
    match_pusher();
    break;
  case 2:
    tobacco_pusher();
    match_pusher();
  }
  Zem_post(&lock);
  return NULL;
}

void *smoker(void *arg) {
  int type = *(int *)arg;
  switch (type) {
  case 0:
    Zem_wait(&tobacco_sem);
    printf("Smoker with tobacco is getting a little bit of cancer.\n");
    Zem_post(&agent_sem);
    break;
  case 1:
    Zem_wait(&paper_sem);
    printf("Smoker with paper is getting a little bit of cancer.\n");
    Zem_post(&agent_sem);
    break;
  case 2:
    Zem_wait(&match_sem);
    printf("Smoker with match is getting a little bit of cancer.\n");
    Zem_post(&agent_sem);
  }
  return NULL;
}

int main() {
  pthread_t agents[3];
  pthread_t smokers[3];
  int types[] = {0, 1, 2};
  init_sem();
  for (int i = 0; i < 3; i++) {
    Pthread_create(&agents[i], NULL, agent, &types[i]);
    Pthread_create(&smokers[i], NULL, smoker, &types[i]);
  }
  for (int i = 0; i < 3; i++) {
    Pthread_join(agents[i], NULL);
    Pthread_join(smokers[i], NULL);
  }
  return 0;
}