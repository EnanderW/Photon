#include <stdlib.h>
#include "arraylist.h"

struct array_list_s {
    void **data;
    int size;
};

array_list *new_array_list() {
    array_list *list = malloc(sizeof(array_list));
    list->data = NULL;
    list->size = 0;
    return list;
}

void init_array_list(array_list *list) {
    list->size = 0;
    list->data = NULL;
}

void delete_array_list(array_list *list) {
    free(list->data);
    free(list);
}

void array_list_add(array_list *list, void *item) {
    int old_size = list->size;
    int new_size = old_size + 1;
    void **old_data = list->data;

    void **new_data = calloc(new_size, sizeof(void*));
    for (unsigned int i = 0; i < old_size; i++)
        new_data[i] = old_data[i];

    new_data[old_size] = item;

    list->data = new_data;
    list->size = new_size;
    free(old_data);
}

void *array_list_remove(array_list *list, unsigned int index) {
    int old_size = list->size;
    void *old_item = array_list_get(list, index);
    if (old_item == NULL) return NULL;

    int new_size = old_size - 1;
    void **old_data = list->data;
    int half = index;

    void **new_data = calloc(new_size, sizeof(void*));

    for (unsigned int i = 0; i < half; i++)
        new_data[i] = old_data[i];

    for (unsigned int i = half; i < old_size - 1; i++)
        new_data[i] = old_data[i + 1];

    list->data = new_data;
    list->size = new_size;
    free(old_data);
    return old_item;
}

int array_list_delete(array_list *list, void *item) {
    int index = array_list_find(list, item);
    if (index == -1) return -1;

    array_list_remove(list, index);
    return index;
}

int array_list_find(array_list *list, void *item) {
    for (int i = 0; i < list->size; i++)
        if (list->data[i] == item) return i;

    return -1;
}

void array_list_clear(array_list *list) {
    free(list->data);
    list->size = 0;
    list->data = NULL;
}

int array_list_size(array_list *list) {
    return list->size;
}

void *array_list_get(array_list *list, unsigned int index) {
    return list->data[index];
}