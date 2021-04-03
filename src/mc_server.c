#include <string.h>
#include <malloc.h>

#include "mc_server.h"
#include "proxy/serverconnection.h"
#include "util/config.h"
#include "util/hashmap.h"

static hashmap *mc_servers;

void *get_mc_server_key(void *value) {
    return ((mc_server *) value)->name;
}

void setup_servers_map() {
    mc_servers = hashmap_new(16, str_hash, str_compare, get_mc_server_key);
}

void add_server(char *name, const char *addr, unsigned short port) {
    int name_size = strlen(name);

    mc_server *new_server = malloc(sizeof(mc_server));
    memcpy(new_server->name, name, name_size);
    new_server->name[name_size] = 0;
    uv_ip4_addr(addr, port, &new_server->server_addr);

    hashmap_put(mc_servers, name, new_server);
}

mc_server *get_server(char *name) {
    return hashmap_get(mc_servers, name);
}

void on_mc_bridge_connect(uv_connect_t *conn, int status) {
    if (status < 0) {
        fprintf(stderr, "Error on connect %s\n", uv_strerror(status));
        return;
    }

    printf("------ > Connected to bridge < ------\n");

    uv_stream_t *stream = conn->handle;
    uv_buf_t *buf = stream->data;

    uv_write_t *req = malloc(sizeof(uv_write_t));
    uv_buf_t wrbuf = uv_buf_init(buf->base, buf->len);
    uv_write(req, stream, &wrbuf, 1, on_write);

    free(conn);
    if (buf->base)
        free(buf->base);

    free(buf);
}

void connect_mc_bridge(mc_server *server, char *buf, int size) {
    uv_tcp_t *socket = malloc(sizeof(uv_tcp_t));
//    uv_tcp_init(loop, socket);
    uv_tcp_keepalive(socket, 1, 60);

    uv_connect_t *connection = malloc(sizeof(uv_connect_t));

    uv_buf_t *data = malloc(sizeof(uv_buf_t));
    data->base = buf;
    data->len = size;
    connection->data = data;

    uv_tcp_connect(connection, socket, (const struct sockaddr *) &server->bridge_addr, on_mc_bridge_connect);
}