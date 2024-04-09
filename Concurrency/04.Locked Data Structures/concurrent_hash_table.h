#ifndef HASH_H
#define HASH_H

#include "concurrent_linked_list.h"

#define BUCKETS (101)

typedef struct __hash_t {
    list_t lists[BUCKETS];
} hash_t;

void Hash_Init(hash_t *H);
int Hash_Insert(hash_t *H, int key, int thread_id);
int Hash_Remove(hash_t *H, int key, int thread_id);

#endif /* HASH_H */
