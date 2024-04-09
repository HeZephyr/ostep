#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include "common_threads.h"

// basic node structure
typedef struct __node_t {
    int key;
    struct __node_t *next;
} node_t;

// basic list structure (one used per list)
typedef struct __list_t {
    node_t *head;
    pthread_mutex_t lock;
} list_t;

void List_Init(list_t *L);
int List_Insert(list_t *L, int key, int thread_id);
int List_Remove(list_t *L, int key, int thread_id);
void List_Print(list_t *L, int thread_id);
void List_Free(list_t *L);

#endif /* LINKED_LIST_H */
