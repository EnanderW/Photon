#include <openssl/sha.h>
#include <string.h>

#include "session/session.h"
#include "phapi/event/event.h"
#include "stage/playerstage.h"
#include "util/config.h"
#include "util/conversion.h"
#include "proxy/connectionrequest.h"
#include "proxy/serverconnection.h"
#include "util/http.h"
#include "proxy/proxycontext.h"

void send_encryption_request(connection_request *request);
void send_compression(connection_request *request);
void send_login_success(connection_request *request);
void callback(connection_request *request, char *json);

void sha_to_mojang(unsigned char *source, char **dest_s) {
    char *dest = *dest_s;
    bool neg = (source[0] >> 7);

    if (neg) {
        for (int i = 0; i < 20; i++)
            source[i] = ~source[i];

        int sum = source[19] + 1;
        for (int i = 19; i >= 1; i--) {
            source[i] = sum;
            sum = source[i - 1] + (sum >> 8);
        }
    }

    int i;
    for (i = 0; i < 20; i++)
        sprintf(dest + (2 * i) + 1, "%02x", source[i]);

    for (i = 1; i < 40; i++)
        if (dest[i] != '0')
            break;

    if (neg) dest[--i] = '-';

    *dest_s = dest + i;
}

void player_login_stage(connection_request *request, char *data, int *index, ssize_t nread) {
    int packet_length = read_var_int(data, index);
    int packet_id = read_var_int(data, index);

    if (packet_id == 0) {
        int username_length = read_var_int(data, index);
        if (username_length > 16) {
            uv_close((uv_handle_t *) request->player_client, on_close);
            free(request->proxy_context);
            connection_request_free_partial(request);
            *index = nread;
            return;
        }

        read_str_b(data, index, username_length, request->username);
        printf("[Login] Username -> %s\n", request->username);

        event_player_login_start event_login = { request, false };
        run_event(EVENT_PLAYER_LOGIN_START, &event_login);
        if (!event_login.canceled)
            send_encryption_request(event_login.request);
    } else if (packet_id == 1) {
        int ssl = read_var_int(data, index);
        unsigned char ss[128];
        read_u_bytes_b(data, index, ssl, ss);

        unsigned char vt[128];
        int vl = read_var_int(data, index);
        read_u_bytes_b(data, index, vl, vt);

        unsigned char token[4];
        rsa_decrypt(token, vt, vl);

        if (memcmp(token, request->verify_token, 4) != 0) {
            uv_close((uv_handle_t *) request->player_client, on_close);
            free(request->proxy_context);
            connection_request_free_partial(request);
            return;
        }

        rsa_decrypt(request->shared_secret, ss, ssl);

        request->encryption_cipher = EVP_CIPHER_CTX_new();
        request->decryption_cipher = EVP_CIPHER_CTX_new();
        EVP_CIPHER_CTX_init(request->encryption_cipher);
        EVP_CIPHER_CTX_init(request->decryption_cipher);

        EVP_EncryptInit_ex(request->encryption_cipher, EVP_aes_128_cfb8(), NULL, request->shared_secret, request->shared_secret);
        EVP_DecryptInit_ex(request->decryption_cipher, EVP_aes_128_cfb8(), NULL, request->shared_secret, request->shared_secret);

        // send mojang http thing
        unsigned char hash[SHA_DIGEST_LENGTH];
        char encoded_s[SHA_DIGEST_LENGTH * 2 + 2] = { 0 };
        char *encoded = encoded_s;

        SHA_CTX ctx;
        SHA1_Init(&ctx);
        SHA1_Update(&ctx, "6f70c7e472f9ddba", strlen("6f70c7e472f9ddba"));
        SHA1_Update(&ctx, request->shared_secret, 16);
        SHA1_Update(&ctx, keys.pub_key, keys.pub_key_size);
        SHA1_Final(hash, &ctx);

        sha_to_mojang(hash, &encoded);

        char params[19 + 16 + 41 + 4 + 15]; // Str + username + encoded + ip (if any)
        if (settings.prevent_proxy_connections) {
            char ip[16];
            int ip_length = 16;
            struct sockaddr sock;
            uv_tcp_getpeername((const uv_tcp_t *) request->player_client, &sock, &ip_length);
            uv_inet_ntop(sock.sa_family, &sock.sa_data, ip, ip_length);

            sprintf(params, "username=%s&serverId=%s&ip=%s", request->username, encoded, ip);
        } else {
            sprintf(params, "username=%s&serverId=%s", request->username, encoded);
        }

        send_request(request, params, callback);
    } else {
        uv_close((uv_handle_t *) request->player_client, on_close);
        free(request->proxy_context);
        connection_request_free_full(request);
        *index = nread;
    }
}

void send_encryption_request(connection_request *request) {
    proxy_context *ctx = request->proxy_context;
    for (int i = 0; i < 4; i++)
        request->verify_token[i] = rand() % 256;

    int packet_id = 0x01;
    int packet_length = get_var_int_size(packet_id) +
                        get_var_int_size(16)
                        + 16 + get_var_int_size(keys.pub_key_size)
                        + keys.pub_key_size + get_var_int_size(4) + 4;

    char *pack = ctx->worker_context->out_data;
    int index = 0;

    write_var_int(pack, &index, packet_length);
    write_var_int(pack, &index, packet_id);
    write_str(pack, &index, 16, "6f70c7e472f9ddba");
    write_var_int(pack, &index, keys.pub_key_size);
    write_bytes(pack, &index, keys.pub_key_size, keys.pub_key);
    write_var_int(pack, &index, 4);
    write_bytes(pack, &index, 4, request->verify_token);

    uv_write_t *req = malloc(sizeof(uv_write_t));
    uv_buf_t wrbuf = uv_buf_init(pack, index);
    uv_write(req, request->player_client, &wrbuf, 1, on_write);
}

void send_compression(connection_request *request) {
    proxy_context *ctx = request->proxy_context;

    int packet_id = 0x03;
    int packet_length = get_var_int_size(packet_id) +
                        get_var_int_size((int)settings.compression_threshold);

    char *pack = ctx->worker_context->out_data;
    int index = 0;

    write_var_int(pack, &index, packet_length);
    write_var_int(pack, &index, packet_id);
    write_var_int(pack, &index, (int)settings.compression_threshold);

    int encrypted_length = index;
    unsigned char *encrypted_data = (unsigned char*) (pack + index);
    aes_encrypt(request->encryption_cipher, (unsigned char*)pack, encrypted_data, &encrypted_length);

    uv_write_t *req = malloc(sizeof(uv_write_t));
    uv_buf_t wrbuf = uv_buf_init((char*) encrypted_data, encrypted_length);
    uv_write(req, request->player_client, &wrbuf, 1, on_write);
}

void send_login_success(connection_request *request) {
    proxy_context *ctx = request->proxy_context;

    int packet_id = 0x02;
    int username_size = strlen(request->username);
    int data_length = get_var_int_size(packet_id) +
                      8 + 8 + // UUID
                      get_var_int_size(username_size) +
                      username_size;
    int packet_length = data_length + get_var_int_size(0);

    char *pack = ctx->worker_context->out_data;
    int index = 0;

    write_var_int(pack, &index, packet_length);
    write_var_int(pack, &index, 0);
    write_var_int(pack, &index, packet_id);
    write_long(pack, &index, request->uuid.most);
    write_long(pack, &index, request->uuid.least);
    write_str(pack, &index, username_size, request->username);

    int encrypted_length = index;
    unsigned char *encrypted_data = (unsigned char*) (pack + index);
    aes_encrypt(request->encryption_cipher, (unsigned char*)pack, encrypted_data, &encrypted_length);

    uv_write_t *req = malloc(sizeof(uv_write_t));
    uv_buf_t wrbuf = uv_buf_init((char*) encrypted_data, encrypted_length);
    uv_write(req, request->player_client, &wrbuf, 1, on_write);
}

void callback(connection_request *request, char *json) {
    if (json == NULL || strlen(json) <= 0) {
        uv_close((uv_handle_t *) request->player_client, on_close);
        free(request->proxy_context);
        connection_request_free_full(request);
        return;
    }

    // UUID -> 32 string size
    // Cheeky way of getting the uuid from the json
    char *uuid_start = strstr(json, "\"id\" : \"") + 8;
    char uuid_most[17];
    char uuid_least[17];
    memcpy(uuid_most, uuid_start, 16);
    memcpy(uuid_least, uuid_start + 16, 16);
    uuid_most[16] = 0;
    uuid_least[16] = 0;

    request->uuid.most = (uint64_t) strtoull(uuid_most, NULL, 16);
    request->uuid.least = (uint64_t) strtoull(uuid_least, NULL, 16);

    send_compression(request);
    send_login_success(request);

    // connect to mc server
    event_player_first_server_connection event_connection = { request, settings.main_server, false };
    run_event(EVENT_PLAYER_FIRST_SERVER_CONNECTION, &event_connection);
    if (!event_connection.canceled)
        connect_first_server(event_connection.request, event_connection.server);
}