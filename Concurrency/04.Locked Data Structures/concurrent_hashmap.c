#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "concurrent_hashmap.h"

hashmap_t* create_hashmap() {
    hashmap_t* hashmap = (hashmap_t*)malloc(sizeof(hashmap_t));
    for (int i = 0; i < NUM_SEGMENTS; ++i) {
        segment_t* segment = &hashmap->segments[i];
        segment->entries = (entry_t**)calloc(NUM_BUCKETS, sizeof(entry_t*));
        pthread_mutex_init(&segment->lock, NULL);
    }
    return hashmap;
}

unsigned int hash(const char* key) {
    unsigned int hash = 0;
    for (int i = 0; key[i]; ++i) {
        hash = hash * 31 + key[i];
    }
    return hash % NUM_BUCKETS;
}

void put(hashmap_t* hashmap, const char* key, const char* value) {
    unsigned int hash_val = hash(key);
    segment_t* segment = &hashmap->segments[hash_val % NUM_SEGMENTS];
    pthread_mutex_lock(&segment->lock);
    entry_t* entry = segment->entries[hash_val];
    if (entry == NULL) {
        entry = (entry_t*)malloc(sizeof(entry_t));
        entry->key = strdup(key);
        segment->entries[hash_val] = entry;
    } else {
        free(entry->value); // Free existing value if it exists
    }
    entry->value = strdup(value);
    pthread_mutex_unlock(&segment->lock);
}

char* get(hashmap_t* hashmap, const char* key) {
    unsigned int hash_val = hash(key);
    segment_t* segment = &hashmap->segments[hash_val % NUM_SEGMENTS];
    pthread_mutex_lock(&segment->lock);
    entry_t* entry = segment->entries[hash_val];
    pthread_mutex_unlock(&segment->lock);
    if (entry == NULL) {
        return NULL;
    }
    return entry->value;
}

void destroy_hashmap(hashmap_t* hashmap) {
    for (int i = 0; i < NUM_SEGMENTS; ++i) {
        segment_t* segment = &hashmap->segments[i];
        pthread_mutex_destroy(&segment->lock);
        for (int j = 0; j < NUM_BUCKETS; ++j) {
            entry_t* entry = segment->entries[j];
            if (entry != NULL) {
                free(entry->key);
                free(entry->value);
                free(entry);
            }
        }
        free(segment->entries);
    }
    free(hashmap);
}
