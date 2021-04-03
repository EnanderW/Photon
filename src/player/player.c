#include "player.h"

#include "proxy/playerconnection.h"
#include "proxy/serverconnection.h"
#include "util/config.h"

void free_player(player *player) {
    if (player->s_conn) {
        free(player->s_conn->proxy_context);
        free(player->s_conn);
    }

    if (player->p_conn) {
        free(player->p_conn->proxy_context);
        free(player->p_conn);
    }

    if (player->decryption_cipher) {
        EVP_CIPHER_CTX_cleanup(player->decryption_cipher);
        EVP_CIPHER_CTX_free(player->decryption_cipher);
    }

    if (player->encryption_cipher) {
        EVP_CIPHER_CTX_cleanup(player->encryption_cipher);
        EVP_CIPHER_CTX_free(player->encryption_cipher);
    }

    free(player);
}

void disconnect_player(player *player) {
    uv_close((uv_handle_t *) player->s_conn->client, on_close);
    uv_close((uv_handle_t *) player->p_conn->client, on_close);
}