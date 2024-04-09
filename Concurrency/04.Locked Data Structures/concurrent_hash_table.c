#include <stdio.h>

# include <assert.h>

#include "concurrent_hash_table.h"

void Hash_Init(hash_t *H) {
    int i;
    for (i = 0; i < BUCKETS; i++) {
        List_Init(&H->lists[i]);
    }
}

int Hash_Insert(hash_t *H, int key, int thread_id) {
    int bucket = key % BUCKETS;
    return List_Insert(&H->lists[bucket], key, thread_id);
}

int Hash_Remove(hash_t *H, int key, int thread_id) {
    int bucket = key % BUCKETS;
    return List_Remove(&H->lists[bucket], key, thread_id);
}
