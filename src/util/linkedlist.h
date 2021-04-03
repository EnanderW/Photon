#ifndef PHOTON_LINKEDLIST_H
#define PHOTON_LINKEDLIST_H

#include <stdbool.h>

typedef void (*free_items)(void *value);

typedef struct item_s {
    void *value;

    struct item_s *next, *previous;
} item;

item *item_new(void *value);
void item_free(item *item);

typedef struct linked_list_s {
    item *first;
    item *last;
    int count;
} linked_list;

linked_list *linked_list_new();
void linked_list_free(linked_list *list, free_items free_items);

void linked_list_add(linked_list *list, void *value);
void linked_list_remove_item(linked_list *list, void *value);

void *linked_list_remove_index(linked_list *list, int index);
void linked_list_clear(linked_list *list, bool free_items);

void *linked_list_get(linked_list *list, int index);


#endif //PHOTON_LINKEDLIST_H
