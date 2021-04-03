#ifndef PHOTON_PLAYERCONNECTION_H
#define PHOTON_PLAYERCONNECTION_H

#include <uv.h>
#include "stage/stage.h"
#include "proxy/proxycontext.h"

typedef struct player_s player;

typedef struct player_connection_s {
    proxy_context *proxy_context;
    uv_stream_t *client;
    uv_write_t req;
    player *player;
    handle_player_stage stage;
} player_connection;

#endif //PHOTON_PLAYERCONNECTION_H
