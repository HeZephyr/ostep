#ifndef HASHMAP_H
#define HASHMAP_H

#include <pthread.h>

#define NUM_BUCKETS 1000
#define NUM_SEGMENTS 10

typedef struct {
    char* key;
    char* value;
} entry_t;

typedef struct {
    entry_t** entries;
    pthread_mutex_t lock;
} segment_t;

typedef struct {
    segment_t segments[NUM_SEGMENTS];
} hashmap_t;

hashmap_t* create_hashmap();
unsigned int hash(const char* key);
void put(hashmap_t* hashmap, const char* key, const char* value);
char* get(hashmap_t* hashmap, const char* key);
void destroy_hashmap(hashmap_t* hashmap);

#endif /* HASHMAP_H */
