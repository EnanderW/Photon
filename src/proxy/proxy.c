#include <string.h>
#include "bridge/bridge.h"
#include "util/conversion.h"
#include "proxy.h"
#include "playerconnection.h"
#include "serverconnection.h"
#include "util/config.h"
#include "session/session.h"
#include "player/player.h"
#include "proxy/connectionrequest.h"
#include "stage/playerstage.h"

void alloc_proxy_buffer(uv_handle_t *handle, size_t size, uv_buf_t *buf) {
    proxy_context *ctx = handle->data;
    buf->base = ctx->worker_context->in_data;
    buf->len = sizeof(ctx->worker_context->in_data);
}

static void handle_first_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    proxy_context *ctx = client->data;
    if (nread < 0) {
        if (nread != UV_EOF)
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));

        uv_close((uv_handle_t *) client, on_close);
        free(ctx);
        return;
    }

    int index = 0;
    int id = read_var_int(buf->base, &index);
    if (id == 0) {
        // Bridge connection
        char password[256];
        unsigned char password_size = read_u_byte(buf->base, &index);

        read_str_b(buf->base, &index, password_size, password);

        if (strcmp(password, settings.bridge_password) != 0) {
            printf("A bridge connection was made. Other party failed to provide correct password.\n");
            uv_close((uv_handle_t *) client, on_close);
            free(ctx);
            return;
        }

        uv_buf_t new_buf = uv_buf_init(buf->base + index, nread - index);
        client->read_cb = on_bridge_read;
        on_bridge_read(client, new_buf.len, &new_buf);
    } else {
        // Normal ping or login
        // Could probably create this on login, since we don't actually need it for just a ping/status request.
        connection_request *request = malloc(sizeof(connection_request));
        request->server = NULL;
        request->encryption_cipher = NULL;
        request->decryption_cipher = NULL;
        request->proxy_context = ctx;
        request->player_client = client;
        request->player_stage = player_handshake_stage;
        client->read_cb = on_initial_read;
        ctx->handle = request;
        on_initial_read(client, nread, buf);
    }
}

void on_initial_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    proxy_context *ctx = client->data;
    connection_request *request = ctx->handle;
    if (nread < 0) {
        if (nread != UV_EOF)
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));

        uv_close((uv_handle_t *) client, on_close);
        free(ctx);
        connection_request_free_full(request);
        return;
    }

    int index = 0;
    while (index < nread)
        request->player_stage(request, buf->base, &index, nread);

    if (index != nread) {
        uv_close((uv_handle_t *) client, on_close);
        free(ctx);
        connection_request_free_full(request);
    }
}

void on_proxy_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    proxy_context *ctx = client->data;
    player *player = ctx->handle;
    if (nread < 0) {
        if (nread != UV_EOF)
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));

        wlock_players();

        remove_player(player);
        remove_player_ctx(ctx->worker_context, player);
        disconnect_player(player);
        free_player(player);

        wunlock_players();
        return;
    }

    player->p_conn->stage(player, buf->base, NULL, nread);
}

void on_proxy_connection(uv_stream_t *server, int status) {
    if (status < 0) {
        fprintf(stderr, "Error on connection %s\n", uv_strerror(status));
        return;
    }

    uv_tcp_t *client = malloc(sizeof(uv_tcp_t));
    uv_stream_t *stream = (uv_stream_t *) client;

    uv_tcp_init(server->loop, client);
    if (uv_accept(server, (uv_stream_t *) client) == 0) {
        worker_context *worker_context = server->data;
        proxy_context *context = malloc(sizeof(proxy_context));
        context->worker_context = worker_context;
        context->handle = NULL;
        stream->data = context;

        uv_read_start(stream, alloc_proxy_buffer, handle_first_read);
    } else {
        uv_close((uv_handle_t *) client, on_close);
    }
}