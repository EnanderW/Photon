#include <stdbool.h>
#include <stdlib.h>

#include "multiplex.h"

#if !defined(WIN32)
#include <unistd.h>
#endif

typedef struct {
    uv_pipe_t peer_handle;
    uv_write_t write_req;
} ipc_peer_t;

static void on_ipc_close(uv_handle_t *handle) {
    ipc_peer_t *pc = handle->data;
    free(pc);
}

static void on_ipc_write(uv_write_t *req, int status) {
    if (status != 0) {
        printf("Error here buddy\n");
        return;
    }

    ipc_peer_t *pc = req->data;
    pc->peer_handle.data = pc;
    uv_close((uv_handle_t *) &pc->peer_handle, on_ipc_close);
}

static void on_pipe_connection(uv_stream_t *pipe, int status) {
    if (status != 0) {
        printf("[s] Error -> %s\n", uv_err_name(status));
        return;
    }

    multiplex_t *multiplex = pipe->data;

    int r = -1;

    uv_buf_t buf = uv_buf_init("W", 1);

    ipc_peer_t *pc = calloc(1, sizeof(*pc));

    if (pipe->type != UV_NAMED_PIPE) return;

    r = uv_pipe_init(pipe->loop, &pc->peer_handle, 1);
    if (r != 0) return;

    do {
        r = uv_accept(pipe, (uv_stream_t *) &pc->peer_handle);
        if (r == 0) break;
        else if (-r == EAGAIN) return;
        else {
            printf("Error -> %s\n", uv_err_name(r));
        }
    } while (true);

    pc->write_req.data = pc;
    r = uv_write2(&pc->write_req, (uv_stream_t*) &pc->peer_handle, &buf, 1, (uv_stream_t*) multiplex->listener, on_ipc_write);
    if (r != 0) {
        printf("[W] Error -> %s\n", uv_err_name(r));
        exit(r);
    }
}

int multiplex_init(multiplex_t *multiplex, uv_tcp_t *listener, const char *pipe_name, unsigned char num_workers, void (*on_worker_start)(void *uv_tcp)) {
    multiplex->listener = listener;
    multiplex->pipe_name = pipe_name;
    multiplex->num_workers = num_workers;
    multiplex->workers_connected = 0;
    multiplex->workers = calloc(num_workers, sizeof(multiplex_worker_t));
    multiplex->on_worker_start = on_worker_start;
    uv_mutex_init(&multiplex->lock);

    unlink(pipe_name);;

    unsigned char i;
    for (i = 0; i < num_workers; i++) {
        multiplex_worker_t *worker = &multiplex->workers[i];
        worker->multiplex = multiplex;
        uv_sem_init(&worker->semaphore, 0);
    }

    return 0;
}

int multiplex_dispatch(multiplex_t *multiplex) {
    int r;

    if (!multiplex->listener->loop) return 1;

    r = uv_pipe_init(multiplex->listener->loop, &multiplex->pipe, 0);
    if (r != 0) return r;

    r = uv_pipe_bind(&multiplex->pipe, multiplex->pipe_name);
    if (r != 0) return r;

    multiplex->pipe.data = multiplex;
    r = uv_listen((uv_stream_t *) &multiplex->pipe, 128, on_pipe_connection);
    if (r != 0) return r;

    unsigned char i;
    for (i = 0; i < multiplex->num_workers; i++)
        uv_sem_post(&multiplex->workers[i].semaphore);

    while (true) {
        uv_mutex_lock(&multiplex->lock);
        int e = uv_run(multiplex->listener->loop, UV_RUN_NOWAIT);
        if (e == 0) break;

        uv_mutex_unlock(&multiplex->lock);
    }

    unlink(multiplex->pipe_name);
    return 0;
}