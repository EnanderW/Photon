#include "event.h"

#include <stdio.h>

#include "util/arraylist.h"
#include "util/linkedlist.h"

static array_list *event_listeners = NULL;
void init_listeners() {
    event_listeners = new_array_list();

    for (int i = 0; i < 5; i++) register_event();
}

unsigned int register_event() {
    array_list_add(event_listeners, linked_list_new());
    return array_list_size(event_listeners) - 1;
}

void run_event(unsigned int event_id, void *event) {
    linked_list *listeners = array_list_get(event_listeners, event_id);

    item *it;
    for (it = listeners->first; it != NULL; it = it->next) {
        listener listener = it->value;
        listener(event);
    }
}

void add_listener(unsigned int event_id, listener listener) {
    linked_list *listeners = array_list_get(event_listeners, event_id);
    linked_list_add(listeners, listener);
}


