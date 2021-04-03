#include "stage/serverstage.h"
#include "util/config.h"
#include "player/player.h"
#include "proxy/serverconnection.h"
#include "proxy/playerconnection.h"

void server_play_stage(player *player, char *data, int *index, ssize_t nread) {
    int encrypted_length = nread;
    proxy_context *ctx = player->s_conn->proxy_context;

    unsigned char *encrypted_data = (unsigned char*) ctx->worker_context->out_data;

    aes_encrypt(player->encryption_cipher, (unsigned char *) data, encrypted_data, &encrypted_length);

//    uv_write_t *req = malloc(sizeof(uv_write_t));
    uv_write_t *req = &player->s_conn->req;
    uv_buf_t wrbuf = uv_buf_init((char *) encrypted_data, encrypted_length);
//    uv_write(req, player->p_conn->client, &wrbuf, 1, on_write);
    uv_write(req, player->p_conn->client, &wrbuf, 1, NULL);
}