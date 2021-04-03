#ifndef PHOTON_WORKERCONTEXT_H
#define PHOTON_WORKERCONTEXT_H

#include <openssl/ssl.h>
#include <uv.h>

#include "util/linkedlist.h"
#include "util/types.h"

typedef struct worker_context_s {
    int id;
    SSL_CTX *ssl_ctx;
    char in_data[2000000];
    char out_data[2000000];
    linked_list *players;
    uv_loop_t *loop;
} worker_context;

void add_player_ctx(worker_context *ctx, player *player);
void remove_player_ctx(worker_context *ctx, player *player);

void init_worker_context(unsigned int workers);
worker_context *worker_context_get(unsigned int index);

#endif //PHOTON_WORKERCONTEXT_H
