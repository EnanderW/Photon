#ifndef PHOTON_ARRAYLIST_H
#define PHOTON_ARRAYLIST_H

typedef struct array_list_s array_list;
struct array_list_s;

void array_list_add(array_list *list, void *item);
void *array_list_remove(array_list *list, unsigned int index);
int array_list_delete(array_list *list, void *item);
void array_list_clear(array_list *list);
int array_list_size(array_list *list);
void *array_list_get(array_list *list, unsigned int index);
int array_list_find(array_list *list, void *item);

array_list *new_array_list();
void delete_array_list(array_list *list);
void init_array_list(array_list *list);

#endif //PHOTON_ARRAYLIST_H
