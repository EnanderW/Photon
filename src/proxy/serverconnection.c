#include <string.h>
#include <session/session.h>
#include "util/conversion.h"
#include "stage/serverstage.h"
#include "proxy.h"
#include "player/player.h"
#include "util/config.h"
#include "proxy/connectionrequest.h"
#include "proxy/proxycontext.h"
#include "proxy/serverconnection.h"
#include "proxy/playerconnection.h"
#include "mc_server.h"

void send_login_packets(uv_stream_t *server_stream, char *username);

void on_server_read(uv_stream_t *tcp, ssize_t nread, const uv_buf_t *buf) {
    proxy_context *ctx = tcp->data;
    player *player = ctx->handle;
    server_connection *s_conn = player->s_conn;
    if (nread < 0) {
        if (nread != UV_EOF)
            fprintf(stderr, "[1] Read error %s\n", uv_err_name(nread));

        // Fallback to fallback server
        if (player->server != settings.fallback_server)
            connect_server(player, settings.fallback_server);
        else {
            wlock_players();

            remove_player(player);
            remove_player_ctx(ctx->worker_context, player);
            disconnect_player(player);
            free_player(player);

            wunlock_players();
        }

        return;
    }

    s_conn->stage(player, buf->base, NULL, nread);
}

void on_switch_server_read(uv_stream_t *tcp, ssize_t nread, const uv_buf_t *buf) {
    proxy_context *ctx = tcp->data;
    switch_request *request = ctx->handle;
    if (nread < 0) {
        if (nread != UV_EOF)
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));

        uv_close((uv_handle_t *) request->server_client, on_close);
        switch_request_free(request);

        // TODO: switch server disconnect event
        return;
    }

    int index = 0;
    while (index < nread)
        request->server_stage(request, buf->base, &index, nread);
}

void on_server_connect(uv_connect_t *conn, int status) {
    switch_request *request = conn->data;
    if (status < 0) {
        fprintf(stderr, "Error on connect %s\n", uv_strerror(status));
        // TODO: server connect fail
        free(request);
        free(conn);
        return;
    }

    uv_stream_t *stream = conn->handle;

    player *player = request->player;
    proxy_context *proxy_context = malloc(sizeof(switch_request));
    proxy_context->handle = request;
    proxy_context->worker_context = player->p_conn->proxy_context->worker_context;

    request->proxy_context = proxy_context;
    stream->data = proxy_context;

    uv_read_start(request->server_client, alloc_proxy_buffer, on_switch_server_read);
    send_login_packets(stream, player->username);

    free(conn);
}

void connect_server(player *player, mc_server *server) {
    uv_tcp_t *socket = malloc(sizeof(uv_tcp_t));
    uv_tcp_init(player->loop, socket);
    uv_tcp_keepalive(socket, 1, 60);

    switch_request *request = malloc(sizeof(switch_request));
    request->proxy_context = NULL;
    request->server_client = (uv_stream_t *) socket;
    request->player = player;
    request->server_stage = server_login_stage;
    request->server = server;

    uv_connect_t *connection = malloc(sizeof(uv_connect_t));
    connection->data = request;
    uv_tcp_connect(connection, socket, (const struct sockaddr *) &server->server_addr, on_server_connect);
}

// ==================================================================================================================================
// ==================================================================================================================================
// ->>>>>>>>>> FIRST SERVER CONNECT
// ==================================================================================================================================
// ==================================================================================================================================

void on_first_server_read(uv_stream_t *tcp, ssize_t nread, const uv_buf_t *buf) {
    proxy_context *ctx = tcp->data;
    connection_request *request = ctx->handle;
    if (nread < 0) {
        if (nread != UV_EOF)
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));

        uv_close((uv_handle_t *) request->player_client, on_close);
        uv_close((uv_handle_t *) request->server_client, on_close);
        connection_request_free_full(request);
        return;
    }

    int index = 0;
    while (index < nread)
        request->server_stage(request, buf->base, &index, nread);
}

void on_first_server_connect(uv_connect_t *conn, int status) {
    connection_request *request = conn->data;
    if (status < 0) {
        fprintf(stderr, "Error on connect %s\n", uv_strerror(status));
        connection_request_free_full(request);
        free(conn);
        return;
    }

    uv_stream_t *stream = conn->handle;

    request->server_client = stream;
    request->server_stage = first_server_login_stage;
    stream->data = request->proxy_context;

    uv_read_start(stream, alloc_proxy_buffer, on_first_server_read);
    send_login_packets(stream, request->username);

    free(conn);
}

void connect_first_server(connection_request *request, mc_server *server) {
    uv_tcp_t *socket = malloc(sizeof(uv_tcp_t));
    uv_tcp_init(request->player_client->loop, socket);
    uv_tcp_keepalive(socket, 1, 60);

    uv_connect_t *connection = malloc(sizeof(uv_connect_t));
    connection->data = request;
    request->server = server;
    uv_tcp_connect(connection, socket, (const struct sockaddr *) &server->server_addr, on_first_server_connect);
}

void send_login_packets(uv_stream_t *server_stream, char *username) {
    // TODO: Use worker context buffers
    {
        int packet_id = 0;
        int protocol = 754;
        char *server_addr = "127.0.0.1"; // For now, does this even matter?
        unsigned short server_port = 25567; // Fow now, does this even matter?
        int pa_status = 2;

        int packet_id_size = get_var_int_size(packet_id);
        int protocol_size = get_var_int_size(protocol);
        int server_addr_size = get_var_int_size((int)strlen(server_addr));
        int pa_status_size = get_var_int_size(pa_status);

        int packet_size = packet_id_size + protocol_size + server_addr_size + ((int) strlen(server_addr)) + sizeof(unsigned short) + pa_status_size;

        char *data = malloc(packet_size + get_var_int_size(packet_size));

        int w_index = 0;
        write_var_int(data, &w_index, packet_size);
        write_var_int(data, &w_index, packet_id);
        write_var_int(data, &w_index, protocol);
        write_str(data, &w_index, (int) strlen(server_addr), server_addr);
        write_u_short(data, &w_index, server_port);
        write_var_int(data, &w_index, pa_status);

        uv_write_t *req = malloc(sizeof(uv_write_t));
        uv_buf_t wrbuf = uv_buf_init(data, packet_size + get_var_int_size(packet_size));
        uv_write(req, server_stream, &wrbuf, 1, on_write);
        free(data);
    }
    // LOGIN ======================================
    {
        int packet_id = 0;

        int username_size = get_var_int_size((int)strlen(username));
        int packet_id_size = get_var_int_size(packet_id);

        int packet_size = packet_id_size+ username_size + (int) strlen(username);

        char *data = malloc(packet_size + get_var_int_size(packet_size));

        int w_index = 0;
        write_var_int(data, &w_index, packet_size);
        write_var_int(data, &w_index, packet_id);
        write_str(data, &w_index, (int) strlen(username), username);

        uv_write_t *req = malloc(sizeof(uv_write_t));
        uv_buf_t wrbuf = uv_buf_init(data, packet_size + get_var_int_size(packet_size));
        uv_write(req, server_stream, &wrbuf, 1, on_write);
        free(data);
    }
}