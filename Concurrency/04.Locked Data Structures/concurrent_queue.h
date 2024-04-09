#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>

typedef struct __node_t {
    int             value;
    struct __node_t *next;
} node_t;

typedef struct __queue_t {
    node_t          *head;
    node_t          *tail;
    pthread_mutex_t head_lock;
    pthread_mutex_t tail_lock;
} queue_t;

void Queue_Init(queue_t *q);
void Queue_Enqueue(queue_t *q, int value);
int Queue_Dequeue(queue_t *q, int *value);

#endif /* QUEUE_H */
