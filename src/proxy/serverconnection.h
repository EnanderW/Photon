#ifndef PHOTON_SERVERCONNECTION_H
#define PHOTON_SERVERCONNECTION_H

#include <uv.h>

#include "stage/stage.h"
#include "util/types.h"

typedef struct server_connection_s {
    proxy_context *proxy_context;
    uv_stream_t *client;
    uv_write_t req;
    player *player;
    handle_server_stage stage;
} server_connection;

void on_server_read(uv_stream_t *tcp, ssize_t nread, const uv_buf_t *buf);
void on_switch_server_read(uv_stream_t *tcp, ssize_t nread, const uv_buf_t *buf);
void on_server_connect(uv_connect_t *conn, int status);
void connect_server(player *player, mc_server *server);

void on_first_server_read(uv_stream_t *tcp, ssize_t nread, const uv_buf_t *buf);
void on_first_server_connect(uv_connect_t *conn, int status);
void connect_first_server(connection_request *request, mc_server *server);

#endif //PHOTON_SERVERCONNECTION_H
