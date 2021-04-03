#ifndef PHOTON_MULTIPLEX_H
#define PHOTON_MULTIPLEX_H

#include <uv.h>

typedef struct multiplex_worker_s multiplex_worker_t;

typedef struct {
    const char *pipe_name;
    uv_pipe_t pipe;
    uv_tcp_t *listener;
    multiplex_worker_t *workers;
    unsigned char num_workers;
    unsigned char workers_connected;
    void (*on_worker_start)(void *uv_tcp);
    uv_mutex_t lock;
} multiplex_t;

struct multiplex_worker_s {
    multiplex_t *multiplex;
    uv_loop_t loop;
    uv_tcp_t listener;
    uv_sem_t semaphore;
    uv_pipe_t pipe;
    uv_connect_t connect_req;
    uv_thread_t thread;
};

int multiplex_init(multiplex_t *multiplex, uv_tcp_t *listener, const char *pipe_name, unsigned char num_workers, void (*on_worker_start)(void *uv_tcp));

int multiplex_worker_create(multiplex_t *multiplex, unsigned char worker_id, void *udata);
int multiplex_dispatch(multiplex_t *multiplex);
#endif //PHOTON_MULTIPLEX_H
