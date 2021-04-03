#ifndef PHOTON_HASHMAP_H
#define PHOTON_HASHMAP_H

#include "linkedlist.h"
#include <stdbool.h>

typedef int (*hash_function)(void *key);
typedef bool (*compare_function)(void *a, void *b);
typedef void* (*get_key_function)(void *value);

struct entry_s;
struct hashmap_s;

typedef struct hashmap_s hashmap;
typedef struct entry_s entry;

hashmap *hashmap_new(unsigned int initial_size, hash_function hash, compare_function compare, get_key_function get_key_function);

void hashmap_free(hashmap *map);

void hashmap_put(hashmap *map, void *key, void *value);
void *hashmap_remove(hashmap *map, void *key);
void hashmap_clear(hashmap *map);
void *hashmap_get(hashmap *map, void *key);
unsigned int hashmap_size(hashmap *map);

int str_hash(void *key);
int uuid_hash(void *key);
bool str_compare(void *a, void *b);
bool uuid_compare(void *a, void *b);

#endif //PHOTON_HASHMAP_H
