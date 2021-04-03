#ifndef PHOTON_EVENT_H
#define PHOTON_EVENT_H

#include <stdbool.h>
#include "util/types.h"

typedef void (*listener)(void*);

void init_listeners();
unsigned int register_event();
void run_event(unsigned int event_id, void *event);
void add_listener(unsigned int event_id, listener listener);

#define EVENT_PLAYER_LOGIN_START 0
#define EVENT_PLAYER_FIRST_SERVER_CONNECTION 1
#define EVENT_PLAYER_SWITCH_SERVER 2
#define EVENT_BRIDGE_MESSAGE 3
#define EVENT_STATUS_PING 4

/*
 *
 * player disconnect / crash
 * server disconnect / crash
 * fail server connect
 * fail switch server connect
 * fail first server connect
 * load settings
 * remove player?
 * add player?
 * proxy start
 * proxy loaded (after tcp)
 * proxy stop
 *
 *
 */

typedef struct event_player_login_start_s {
    connection_request *request;
    bool canceled;
} event_player_login_start;

typedef struct event_player_first_server_connection_s {
    connection_request *request;
    mc_server *server;
    bool canceled;
} event_player_first_server_connection;

typedef struct event_player_switch_server_s {
    player *player;
    mc_server *from;
    mc_server *to;
    bool canceled;
} event_player_switch_server;

typedef struct event_bridge_message_s {
    proxy_context *ctx;
    unsigned char packet_name_size;
    char *packet_name;
    char *data;
    unsigned int nread;
} event_bridge_message;

typedef struct event_status_ping_s {
    int max_players;
    int players;
    char *sample;
    char *name;
    int protocol;
    char *description;
    char *favicon;
} event_status_ping;

#endif //PHOTON_EVENT_H
