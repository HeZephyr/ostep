#ifndef HAND_OVER_HAND_LOCKING_LIST_H
#define HAND_OVER_HAND_LOCKING_LIST_H

#include <pthread.h>
// basic node structure
typedef struct __node_t {
    int key;
    struct __node_t *next;  // pointer to next node
    pthread_mutex_t lock;   // lock for this node
} node_t;

// basic list structure (one used per list)
typedef struct __list_t {
    node_t *head;
    pthread_mutex_t insert_lock;   // insert lock
} list_t;

// Function to initialize a list
void List_Init(list_t *L);

// Function to insert a new element with key into the list
int List_Insert(list_t *L, int key);

// Function to remove the element with key from the list
int List_Remove(list_t *L, int key);

// Function to print the list
void List_Print(list_t *L);

#endif /* HAND_OVER_HAND_LOCKING_LIST_H */