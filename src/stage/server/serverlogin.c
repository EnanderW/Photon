#include <string.h>

#include "util/config.h"
#include "proxy/proxy.h"
#include "stage/serverstage.h"
#include "util/conversion.h"
#include "proxy/connectionrequest.h"
#include "player/player.h"
#include "stage/playerstage.h"
#include "session/session.h"
#include "proxy/proxycontext.h"
#include "proxy/serverconnection.h"
#include "proxy/playerconnection.h"

void send_respawn_packets(EVP_CIPHER_CTX *cipher, player_connection *conn, char *packet, int *read_index);

void disconnect_existing_player(uv_async_t *handle) {
    player *player = handle->data;

    wlock_players();
    disconnect_player(player);
    free_player(player);

    wunlock_players();

    free(handle);
}

void server_login_stage(switch_request *request, char *data, int *index, ssize_t nread) {
    int packet_length = read_var_int(data, index);
    int packet_id = read_var_int(data, index);

    if (packet_id == 0x03) { // Compression packet
        int threshold = read_var_int(data, index); // Ignore this, maybe add support for it later
        request->server_stage = server_login_stage_compression;
    }
}

void server_login_stage_compression(switch_request *request, char *data, int *index, ssize_t nread) {
    int start_index = *index;

    int packet_length = read_var_int(data, index);
    int data_length = read_var_int(data, index);

    bool compressed = false;
    int packet_id;
    char *packet;
    int read_index = 0;

    if (data_length == 0) { // Not compressed
        packet_id = read_var_int(data, index);
        packet = data + (*index);
        *index += packet_length - get_var_int_size(data_length) - get_var_int_size(packet_id);
    } else { // Compressed
        compressed = true;
        int remaining_bytes = packet_length - get_var_int_size(data_length);

        long source_length = remaining_bytes;
        char *source = data + (*index);
        packet = photon_uncompress(source, source_length, data_length);  // TODO: Should probably use worker_context out_data

        packet_id = read_var_int(packet, &read_index);

        *index += remaining_bytes;
    }

    // We ignore the Login Success packet
    if (packet_id == 0x24) { // Join Game
        player *player = request->player;
        player->server = request->server;

        request->proxy_context->handle = player;

        // Create new server connection
        server_connection *server_connection = malloc(sizeof(struct server_connection_s));
        server_connection->proxy_context = request->proxy_context;
        server_connection->client = request->server_client;
        server_connection->client->read_cb = on_server_read;
        server_connection->client->data = request->proxy_context;
        server_connection->stage = server_play_stage;
        server_connection->player = player;

        // Close old connection
        uv_close((uv_handle_t *) player->s_conn->client, on_close);
        wlock_players();
        free(player->s_conn->proxy_context);
        free(player->s_conn);
        player->s_conn = server_connection;
        wunlock_players();

        // Send join game packet
        {
            int total_size = *index - start_index;
            char *join_packet = malloc(sizeof(char) * total_size);
            memcpy(join_packet, data + start_index, total_size);

            uv_buf_t remaining_buf = uv_buf_init(join_packet, total_size);
            on_server_read(server_connection->client, total_size, &remaining_buf);
            free(join_packet);
        }

        send_respawn_packets(player->encryption_cipher, player->p_conn, packet, &read_index);

        // Send rest of data to on_server_read
        int remaining_bytes_size = nread - (*index);
        if (remaining_bytes_size > 0) {
            uv_buf_t remaining_buf = uv_buf_init(data + *index, remaining_bytes_size);
            on_server_read(server_connection->client, remaining_bytes_size, &remaining_buf);
        }

        free(request);
        *index = nread; // Exit read loop
    }

    if (compressed)
        free(packet);
}

void first_server_login_stage(connection_request *request, char *data, int *index, ssize_t nread) {
    int packet_length = read_var_int(data, index);
    int packet_id = read_var_int(data, index);

    if (packet_id == 0x03) { // Compression packet
        int threshold = read_var_int(data, index); // All servers must share the same threshold as the proxy. Change in config.set
        request->server_stage = first_server_login_stage_compression;
    }
}

void first_server_login_stage_compression(connection_request *request, char *data, int *index, ssize_t nread) {
    int packet_length = read_var_int(data, index);
    int data_length = read_var_int(data, index);

    bool compressed = false;
    int packet_id;
    char *packet;

    if (data_length == 0) { // Not compressed
        packet_id = read_var_int(data, index);
        packet = data + (*index);
        *index += packet_length - get_var_int_size(data_length) - get_var_int_size(packet_id);
    } else { // Compressed
        compressed = true;
        int remaining_bytes = packet_length - get_var_int_size(data_length);

        long source_length = remaining_bytes;
        char *source = data + (*index);
        packet = photon_uncompress(source, source_length, data_length); // TODO: Should probably use worker_context out_data

        int de_index = 0;
        packet_id = read_var_int(packet, &de_index);

        *index += remaining_bytes;
    }

    if (packet_id == 0x02) { // Login success
        proxy_context *player_context = malloc(sizeof(proxy_context));
        proxy_context *server_context = malloc(sizeof(proxy_context));

        player *_player = malloc(sizeof(struct player_s));
        _player->worker_context = request->proxy_context->worker_context;
        _player->loop = request->player_client->loop;
        _player->server = request->server;
        _player->uuid = request->uuid;
        _player->encryption_cipher = request->encryption_cipher;
        _player->decryption_cipher = request->decryption_cipher;
        memcpy(_player->username, request->username, 17);

        player_context->worker_context = _player->worker_context;
        server_context->worker_context = _player->worker_context;
        player_context->handle = _player;
        server_context->handle = _player;

        // Create _player connection
        player_connection *player_connection = malloc(sizeof(struct player_connection_s));
        player_connection->proxy_context = player_context;
        player_connection->client = request->player_client;
        player_connection->client->read_cb = on_proxy_read;
        player_connection->client->data = player_context;
        player_connection->stage = player_play_stage;
        player_connection->player = _player;

        // Create server connection
        server_connection *server_connection = malloc(sizeof(struct server_connection_s));
        server_connection->proxy_context = server_context;
        server_connection->client = request->server_client;
        server_connection->client->read_cb = on_server_read;
        server_connection->client->data = server_context;
        server_connection->stage = server_play_stage;
        server_connection->player = _player;

        _player->s_conn = server_connection;
        _player->p_conn = player_connection;

        wlock_players();
        player *existing = get_player_s(request->username);
        uv_loop_t *existing_loop = NULL;
        if (existing) {
            existing_loop = existing->loop;
            remove_player(existing);
            remove_player_ctx(existing->worker_context, existing);

            existing->username[0] = 0;
            existing->username[1] = 0;
            existing->username[2] = 0;

            if (existing_loop != NULL && existing->server != request->server) {
                if (existing->worker_context->id != _player->worker_context->id) {
                    uv_async_t *event = malloc(sizeof(uv_async_t));
                    uv_async_init(existing_loop, event, disconnect_existing_player);
                    event->data = existing;
                    uv_async_send(event);
                } else {
                    disconnect_player(existing);
                    free_player(existing);
                }
            }
        }

        add_player(_player);
        add_player_ctx(_player->worker_context, _player);
        wunlock_players();

        // Send rest of data to on_server_read
        int remaining_bytes_size = nread - (*index);
        if (remaining_bytes_size > 0) {
            uv_buf_t remaining_buf = uv_buf_init(data + *index, remaining_bytes_size);
            on_server_read(server_connection->client, remaining_bytes_size, &remaining_buf);
        }

        free(request->proxy_context);
        connection_request_free_partial(request);

        *index = nread; // Exit read loop
    }

    if (compressed)
        free(packet);
}

void send_respawn_packets(EVP_CIPHER_CTX *cipher, player_connection *conn, char *data, int *index) {
    proxy_context *ctx = conn->proxy_context;

    int entity_id = read_int(data, index);

    bool hardcore = read_bool(data, index);
    unsigned char gamemode = read_u_byte(data, index);
    char prev_gamemode = read_byte(data, index);
    int world_count = read_var_int(data, index);
    for (int i = 0; i < world_count; i++) {
        char *world_name = read_var_str(data, index);
        free(world_name);
    }

    // Dimension codec and dimension
    skip_tag(data, index);
    int start_dimension = *index;
    skip_tag(data, index);
    int end_dimension = *index;

    char *world_name = read_var_str(data, index);

    long seed = read_long(data, index);
    int max_players = read_var_int(data, index);
    int view_distance = read_var_int(data, index);
    bool reduced_debug_info = read_bool(data, index);
    bool enabled_respawn_screen = read_bool(data, index);
    bool is_debug = read_bool(data, index);
    bool is_flat = read_bool(data, index);

    if (true) { // SEND RESPAWN
        int packet_id = 0x39;
        int dimension_size = end_dimension - start_dimension;
        int world_name_size = strlen(world_name);

        char *packet_data = ctx->worker_context->out_data;
        int de_index = 0;

        write_var_int(packet_data, &de_index, packet_id);
        write_bytes(packet_data, &de_index, dimension_size, data + start_dimension);
        write_str(packet_data, &de_index, world_name_size, world_name);
        write_long(packet_data, &de_index, seed);
        write_byte(packet_data, &de_index, gamemode);
        write_byte(packet_data, &de_index, prev_gamemode);
        write_byte(packet_data, &de_index, is_debug);
        write_byte(packet_data, &de_index, is_flat);
        write_byte(packet_data, &de_index, 0x00);

        int data_length = de_index;
        int compressed_size = data_length;
        char *compressed = packet_data + de_index;
        photon_compress(data_length, packet_data, &compressed_size, compressed);

        de_index += compressed_size;
        char *full_packet = packet_data + de_index;
        int packet_length = compressed_size + get_var_int_size(data_length);

        int total_start = de_index;
        write_var_int(packet_data, &de_index, packet_length);
        write_var_int(packet_data, &de_index, data_length);
        write_bytes(packet_data, &de_index, compressed_size, compressed);
        int total_size = de_index - total_start;

        {
            int encrypted_length = total_size;
            char *encrypted_data = packet_data + de_index;
            aes_encrypt(cipher, (unsigned char*)full_packet, (unsigned char*) encrypted_data, &encrypted_length);

            uv_write_t *req = malloc(sizeof(uv_write_t));
            uv_buf_t wrbuf = uv_buf_init(encrypted_data, encrypted_length);
            uv_write(req, conn->client, &wrbuf, 1, on_write);
        }
    }

    free(world_name);
}