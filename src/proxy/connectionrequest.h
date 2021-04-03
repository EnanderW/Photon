#ifndef PHOTON_CONNECTIONREQUEST_H
#define PHOTON_CONNECTIONREQUEST_H

#include <uv.h>

#include "stage/stage.h"
#include "util/types.h"
#include "player/uuid.h"

typedef struct connection_request_s {
    proxy_context *proxy_context;
    uv_stream_t *player_client;
    uv_stream_t *server_client;
    handle_request_stage player_stage;
    handle_request_stage server_stage;
    unsigned char shared_secret[16];
    unsigned char verify_token[4];
    EVP_CIPHER_CTX *encryption_cipher;
    EVP_CIPHER_CTX *decryption_cipher;
    char username[17];
    uuid uuid;
    mc_server *server;
} connection_request;

void connection_request_free_full(connection_request *request);
void connection_request_free_partial(connection_request *request);

typedef struct switch_request_s {
    proxy_context *proxy_context;
    uv_stream_t *server_client;
    handle_switch_stage server_stage;
    player *player;
    mc_server *server;
} switch_request;

void switch_request_free(switch_request *request);

#endif //PHOTON_CONNECTIONREQUEST_H
