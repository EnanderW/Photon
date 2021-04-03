#include "workercontext.h"

static worker_context *worker_contexts;

void init_worker_context(unsigned int workers) {
    worker_contexts = calloc(workers, sizeof(worker_context));
}

worker_context *worker_context_get(unsigned int index) {
    return &worker_contexts[index];
}

void add_player_ctx(worker_context *ctx, player *player) {
    linked_list_add(ctx->players, player);
}
void remove_player_ctx(worker_context *ctx, player *player) {
    linked_list_remove_item(ctx->players, player);
}