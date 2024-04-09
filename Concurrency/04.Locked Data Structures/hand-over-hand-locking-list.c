#include "hand-over-hand-locking-list.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Function to initialize a list
void List_Init(list_t *L) {
    L->head = NULL;
    pthread_mutex_init(&L->insert_lock, NULL); // Initialize lock for the list
}

// Function to insert a new element with key into the list
int List_Insert(list_t *L, int key) {
    node_t *new_node = (node_t *)malloc(sizeof(node_t));
    if (new_node == NULL) {
        perror("malloc");
        return -1; // failure
    }
    new_node->key = key;
    pthread_mutex_init(&new_node->lock, NULL); // Initialize lock for the new node

    // just lock the critical section
    pthread_mutex_lock(&L->insert_lock); // Lock the list
    new_node->next = L->head;
    L->head = new_node;
    pthread_mutex_unlock(&L->insert_lock); // Unlock the list
    return 0; // success
}

// Function to remove the element with key from the list
int List_Remove(list_t *L, int key) {
    node_t *curr = L->head;
    node_t *prev = NULL;
    if (curr == NULL)
        return -1; // failure
    pthread_mutex_lock(&curr->lock); // Lock the head node
    while (curr) {
        if (curr->key == key) {
            if (prev)
                prev->next = curr->next;
            else
                L->head->next = curr->next;
            pthread_mutex_unlock(&curr->lock); // Unlock the head node
            free(curr);
            return 0; // success
        }
        prev = curr;
        curr = curr->next;
        if (curr)
            pthread_mutex_lock(&curr->lock); // Lock the next node
        if (prev)
            pthread_mutex_unlock(&prev->lock); // Unlock the previous node
    }
    return -1; // failure
}

// Function to print the list
void List_Print(list_t *L) {
    node_t *curr = L->head;
    if (curr == NULL) {
        printf("List is empty\n");
        return;
    }
    pthread_mutex_lock(&curr->lock); // Lock the head node
    while (curr) {
        printf("%d ", curr->key);
        node_t *next = curr->next;
        pthread_mutex_unlock(&curr->lock); // Unlock the current node
        if (next)
            pthread_mutex_lock(&next->lock); // Lock the next node
        curr = next;
    }
    printf("\n");
}

// Function to free the list
void List_Free(list_t *L) {
    node_t *curr = L->head;
    if (curr == NULL)
        return;
    pthread_mutex_lock(&curr->lock); // Lock the head node
    while (curr) {
        node_t *next = curr->next;
        if (next)
            pthread_mutex_lock(&next->lock); // Lock the next node
        pthread_mutex_unlock(&curr->lock); // Unlock the current node
        free(curr);
        curr = next;
    }
    pthread_mutex_destroy(&L->insert_lock); // Destroy the lock for the list
}