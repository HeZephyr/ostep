#include "concurrent_linked_list.h"
#include "common_threads.h"
#include <stdio.h>
#include <stdlib.h>

void List_Init(list_t *L) {
    L->head = NULL;
    pthread_mutex_init(&L->lock, NULL);
}

int List_Insert(list_t *L, int key, int thread_id) {
    printf("Thread %d: Inserting %d\n", thread_id, key);
    node_t *new = malloc(sizeof(node_t));
    if (new == NULL) {
        perror("malloc");
        pthread_mutex_unlock(&L->lock);
        return -1; // fail
    }
    new->key = key;
    pthread_mutex_lock(&L->lock);
    new->next = L->head;
    L->head = new;
    pthread_mutex_unlock(&L->lock);
    return 0; // success
}

int List_Remove(list_t *L, int key, int thread_id) {
    pthread_mutex_lock(&L->lock);
    printf("Thread %d: Removing %d\n", thread_id, key);
    node_t *curr = L->head;
    node_t *prev = NULL;
    while (curr) {
        if (curr->key == key) {
            if (prev) {
                prev->next = curr->next;
            } else {
                L->head = curr->next;
            }
            pthread_mutex_unlock(&L->lock);
            return 0; // success
        }
        curr = curr->next;
    }
    pthread_mutex_unlock(&L->lock);
    return -1; // failure
}

void List_Print(list_t *L, int thread_id) {
    pthread_mutex_lock(&L->lock);
    if (thread_id != -1) {
        printf("Thread %d, List contents: ", thread_id);
    } else {
        printf("Main thread, List contents: ");
    }
    node_t *curr = L->head;
    while (curr) {
        printf("%d ", curr->key);
        curr = curr->next;
    }
    printf("\n");
    pthread_mutex_unlock(&L->lock);
}

void List_Free(list_t *L) {
    pthread_mutex_lock(&L->lock);
    node_t *curr = L->head;
    while (curr) {
        node_t *temp = curr;
        curr = curr->next;
        free(temp);
    }
    pthread_mutex_unlock(&L->lock);
    pthread_mutex_destroy(&L->lock);
}