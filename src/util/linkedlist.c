#include <malloc.h>
#include <stdio.h>
#include "linkedlist.h"

item *item_new(void *value) {
    item *itm = malloc(sizeof(item));

    itm->value = value;
    itm->next = NULL;
    itm->previous = NULL;
    return itm;
}

void item_free(item *item) {
    free(item);
}

linked_list *linked_list_new() {
    linked_list *list = malloc(sizeof(linked_list));

    list->count = 0;
    list->first = NULL;
    list->last = NULL;

    return list;
}

void linked_list_free(linked_list *list, free_items free_items) {
    if (free_items) {
        item *itm = list->first;
        while (itm != NULL) {
            item *old = itm;
            itm = itm->next;
            if (free_items) free_items(old->value);
            item_free(old);
        }
    }

    free(list);
}

void linked_list_unlink(linked_list *list, item *itm) {
    item *prev = itm->previous;
    item *next = itm->next;

    if (prev == NULL) {
        list->first = next;
    } else {
        prev->next = next;
    }

    if (next == NULL) {
        list->last = prev;
    } else {
        next->previous = prev;
    }

    list->count--;
    item_free(itm);
}

void linked_list_add(linked_list *list, void *value) {
    item *itm = item_new(value);
    if (list->first == NULL) {
        list->first = itm;
        list->last = itm;
    } else {
        itm->previous = list->last;
        list->last->next = itm;
        list->last = itm;
    }

    list->count++;
}

void linked_list_remove_item(linked_list *list, void *value) {
    for (item *itm = list->first; itm != NULL; itm = itm->next) {
        if (itm->value == value) {
            linked_list_unlink(list, itm);
            break;
        }
    }
}

void *linked_list_remove_index(linked_list *list, int index) {
    item *itm = list->first;

    for (int i = 0; i < index; i++) {
        itm = itm->next;
        if (itm == NULL) return NULL;
    }

    void *value = itm->value;
    linked_list_unlink(list, itm);
    return value;
}

void linked_list_clear(linked_list *list, bool free_items) {
    item *itm = list->first;
    if (free_items) {
        while (itm != NULL) {
            item *old = itm;
            itm = itm->next;
            free(old->value);
            item_free(old);
        }
    } else {
        while (itm != NULL) {
            item *old = itm;
            itm = itm->next;
            item_free(old);
        }
    }

    list->first = NULL;
    list->last = NULL;
    list->count = 0;
}

void *linked_list_get(linked_list *list, int index) {
    item *item = list->first;

    for (int i = 0; i < index; i++) {
        item = item->next;
        if (item == NULL) return NULL;
    }

    return item->value;
}