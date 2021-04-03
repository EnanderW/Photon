#include <malloc.h>
#include <string.h>
#include "hashmap.h"

#define LOAD_FACTOR 0.58

struct entry_s {
    void *value;
    int hash;
};

struct hashmap_s {
    linked_list *data;
    unsigned int length;
    unsigned int count;
    unsigned int threshold;
    hash_function hash_function;
    compare_function compare_function;
    get_key_function get_key_function;
};

unsigned int hashmap_size(hashmap *map) {
    return map->count;
}

void hashmap_reput(hashmap *map, entry *ent) {
    int hash = ent->hash;
    int index = hash & ((int)map->length - 1);

    linked_list *entries = &map->data[index];
    linked_list_add(entries, ent);
}

static void resize(hashmap *map, unsigned int new_cap) {
    unsigned int old_cap = map->length;
    linked_list *old_data = map->data;
    map->data = calloc(new_cap, sizeof(linked_list));
    map->threshold = (unsigned int) ((double) new_cap * LOAD_FACTOR);
    map->length = new_cap;

    for (unsigned int i = 0; i < old_cap; i++) {
        linked_list *entries = &old_data[i];
        item *first = entries->first;
        if (first == NULL) continue;

        item *item;
        for (item = first; item != NULL; item = item->next) {
            entry *ent = item->value;
            hashmap_reput(map, ent);
        }
    }

    free(old_data);
}

void hashmap_put(hashmap *map, void *key, void *value) {
    int hash = map->hash_function(key);
    int index = hash & ((int)map->length - 1);

    linked_list *entries = &map->data[index];

    entry *ent = malloc(sizeof(entry));
    ent->value = value;
    ent->hash = hash;

    linked_list_add(entries, ent);

    map->count++;

    if (map->count >= map->threshold)
        resize(map, map->length * 2);
}

void *hashmap_remove(hashmap *map, void *key) {
    int hash = map->hash_function(key);
    int index = hash & ((int)map->length - 1);

    linked_list *entries = &map->data[index];
    item *first = entries->first;
    if (first == NULL) return NULL;


    item *item;
    for (item = first; item != NULL; item = item->next) {
        entry *ent = item->value;
        if (map->compare_function(key, map->get_key_function(ent->value))) {
            linked_list_remove_item(entries, ent);
            map->count--;

            void *value = ent->value;
            free(ent);
            return value;
        }
    }

    return NULL;
}

void hashmap_clear(hashmap *map) {

}

void *hashmap_get(hashmap *map, void *key) {
    int hash = map->hash_function(key);
    int index = hash & ((int)map->length - 1);

    linked_list *entries = &map->data[index];

    item *first = entries->first;
    if (first == NULL) return NULL;
    else if (entries->count == 1) return ((entry*)first->value)->value;

    item *item;
    for (item = first; item != NULL; item = item->next) {
        entry *ent = item->value;
        if (map->compare_function(key, map->get_key_function(ent->value))) {
            void *value = ent->value;
            return value;
        }
    }


    return NULL;
}

hashmap *hashmap_new(unsigned int initial_size, hash_function hash, compare_function compare, get_key_function get_key_function) {
    hashmap *map = malloc(sizeof(hashmap));

    map->data = calloc(initial_size, sizeof(linked_list));
    map->hash_function = hash;
    map->compare_function = compare;
    map->get_key_function = get_key_function;
    map->length = initial_size;
    map->count = 0;
    map->threshold = (unsigned int) ((double) initial_size * LOAD_FACTOR);

    return map;
}

void hashmap_free(hashmap *map) {
    free(map->data);
    free(map);
}

int str_hash(void *input) {
    char *key = input;

    int n = strlen(key);
    int hash = 0;
    for (int i = 0; i < n - 1; i++) {
        hash += (key[i] * 31 ^ (n - 1));
    }

    hash += key[n - 1] ^ (n - 1);
    return hash;
}

#include "player/uuid.h"
int uuid_hash(void *key) {
    uuid *uuid = key;
    int64_t hilo = uuid->most ^ uuid->least;
    return ((int)(hilo >> 32)) ^ (int) hilo;
}

bool str_compare(void *a, void *b) {
    return strcmp(a, b) == 0;
}

bool uuid_compare(void *a, void *b) {
    uuid *u1 = a;
    uuid *u2 = b;

    return (u1->most == u2->most && u1->least == u2->least);
}