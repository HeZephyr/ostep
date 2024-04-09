#include "concurrent_queue.h"
#include "common_threads.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

void Queue_Init(queue_t *q) {
    node_t *tmp = malloc(sizeof(node_t));
    tmp->next = NULL;
    q->head = q->tail = tmp;
    Pthread_mutex_init(&q->head_lock, NULL);
    Pthread_mutex_init(&q->tail_lock, NULL);
}

void Queue_Enqueue(queue_t *q, int value) {
    node_t *tmp = malloc(sizeof(node_t));
    assert(tmp != NULL);
    tmp->value = value;
    tmp->next = NULL;

    Pthread_mutex_lock(&q->tail_lock);
    q->tail->next = tmp;
    q->tail = tmp;
    Pthread_mutex_unlock(&q->tail_lock);
}

int Queue_Dequeue(queue_t *q, int *value) {
    Pthread_mutex_lock(&q->head_lock);
    node_t *tmp = q->head;
    node_t *new_head = tmp->next;
    if (new_head == NULL) {
        Pthread_mutex_unlock(&q->head_lock);
        return -1;
    }
    *value = new_head->value;
    q->head = new_head;
    Pthread_mutex_unlock(&q->head_lock);
    free(tmp);
    return 0;
}
