#include "multiplex.h"
#include <stdlib.h>

static char buffer[1];

static void on_ipc_alloc(uv_handle_t *handle, size_t size, uv_buf_t *buf) {
    buf->base = buffer;
    buf->len = 1;
}

static void on_ipc_read(uv_stream_t *handle, ssize_t nread, const uv_buf_t *buf) {
    int r;
    multiplex_worker_t *worker = handle->data;

    uv_handle_type type = uv_pipe_pending_type((uv_pipe_t *) handle);
    int pending_count = uv_pipe_pending_count((uv_pipe_t *) handle);

    if (pending_count == 0) {
        uv_close((uv_handle_t *) handle, NULL);
        return;
    }

    if (uv_pipe_pending_count((uv_pipe_t *) handle) != 1) {
        printf("Ehh?\n");
        return;
    }

    if (type != UV_TCP) {
        printf("Ehh tcp?\n");
        return;
    }

    r = uv_tcp_init(handle->loop, &worker->listener);
    if (r != 0) {
        printf("Err 1\n");
        return;
    }

    r = uv_accept(handle, (uv_stream_t *) &worker->listener);
    if (r != 0) {
        printf("Err 2\n");
        return;
    }

    uv_close((uv_handle_t *) handle, NULL);
}

static void on_ipc_connect(uv_connect_t *req, int status) {
    int r;
    multiplex_worker_t *worker = req->data;

    if (status != 0) {
        printf("Some error\n");
        return;
    }

    req->handle->data = worker;
    r = uv_read_start((uv_stream_t*) &worker->pipe, on_ipc_alloc, on_ipc_read);
    if (r != 0) {
        printf("RE -> %s\n", uv_err_name(r));
    }

    // free(req); ?
}

static void last_worker_cleanup(multiplex_worker_t *worker) {
    uv_mutex_lock(&worker->multiplex->lock);
    worker->multiplex->workers_connected += 1;

    if (worker->multiplex->workers_connected == worker->multiplex->num_workers) {
        uv_close((uv_handle_t *) &worker->multiplex->pipe, NULL);
        uv_close((uv_handle_t *) worker->multiplex->listener, NULL);
    }

    uv_mutex_unlock(&worker->multiplex->lock);
}

static void get_listen_handle(uv_loop_t *loop, multiplex_worker_t *worker) {
    do {
        int r = uv_pipe_init(loop, &worker->pipe, 1);
        if (r != 0) {
            printf("Error here too buddy.\n");
            return;
        }

        worker->connect_req.data = worker;
        uv_pipe_connect(&worker->connect_req, &worker->pipe, worker->multiplex->pipe_name, on_ipc_connect);

        r = uv_run(loop, UV_RUN_DEFAULT);
        if (r != 0) {
            printf("error -> %s\n", uv_err_name(r));
        }
    } while (!worker->listener.loop);

    last_worker_cleanup(worker);
}

static void worker_entry(void *_worker) {
    multiplex_worker_t *worker = _worker;

    if (!worker) {
        printf("No worker\n");
        return;
    }

    if (worker->listener.loop) {
        printf("Is loop\n");
        return;
    }

    uv_sem_wait(&worker->semaphore);
    uv_sem_destroy(&worker->semaphore);

    get_listen_handle(&worker->loop, worker);

    worker->multiplex->on_worker_start(&worker->listener);
}

int multiplex_worker_create(multiplex_t *multiplex, unsigned char worker_id, void *udata) {
    int r;
    multiplex_worker_t *worker = &multiplex->workers[worker_id];
    worker->listener.data = udata;

    r = uv_loop_init(&worker->loop);
    if (r != 0) return r;

    r = uv_thread_create(&worker->thread, worker_entry, worker);
    return r;
}