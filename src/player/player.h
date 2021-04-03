#ifndef PHOTON_PLAYER_H
#define PHOTON_PLAYER_H

#include <uv.h>

#include "util/types.h"
#include "uuid.h"

typedef struct player_s {
    player_connection *p_conn;
    server_connection *s_conn;
    mc_server *server;
    char username[17];
    uuid uuid;

    EVP_CIPHER_CTX *encryption_cipher;
    EVP_CIPHER_CTX *decryption_cipher;

    uv_loop_t *loop;
    worker_context *worker_context;
} player;

void free_player(player *player);
void disconnect_player(player *player);

#endif //PHOTON_PLAYER_H
