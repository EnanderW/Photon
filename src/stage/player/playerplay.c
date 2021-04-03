#include "stage/playerstage.h"
#include "player/player.h"
#include "util/config.h"
#include "proxy/playerconnection.h"
#include "proxy/serverconnection.h"

void player_play_stage(player *player, char *data, int *index, ssize_t nread) {
    int decrypted_length = nread;
    proxy_context *ctx = player->p_conn->proxy_context;
    unsigned char *decrypted_data = (unsigned char*) ctx->worker_context->out_data;

    aes_decrypt(player->decryption_cipher, (unsigned char*) data, decrypted_data, &decrypted_length);

//    uv_write_t *req = malloc(sizeof(uv_write_t));
    uv_write_t *req = &player->p_conn->req;
    uv_buf_t wrbuf = uv_buf_init((char*) decrypted_data, decrypted_length);
//    uv_write(req, player->s_conn->client, &wrbuf, 1, on_write);
    uv_write(req, player->s_conn->client, &wrbuf, 1, NULL);
}