#include <stdio.h>
#include <time.h>

#include "util/plugin.h"
#include "proxy/proxycontext.h"
#include "proxy/proxy.h"
#include "mc_server.h"
#include "util/config.h"
#include "session/session.h"
#include "multiplex/multiplex.h"
#include "util/http.h"
#include "phapi/event/event.h"

/*
 * Stuff to do/work on
 *
 * API
 * THREAD SAFETY (Fixed, but we should try to make sure, somehow)
 * MEMORY LEAKS (Fixed, but maybe a few hidden ones still)
 * PROXY -> SERVER CONNECTION (how?) (possibility to send through player plugin channel, but should we support a non player connection?)
 * HANDLE BAD PACKETS
 *
 */

#if defined(WIN32)
#define PIPE_NAME "\\\\?\\pipe\\photon.sock"
#else
#define PIPE_NAME "/tmp/photon.sock"
#endif

static void on_worker_start(void *tcp) {
    uv_tcp_t *listener = tcp;
    int r = uv_listen((uv_stream_t *) listener, (int) (settings.max_connections / settings.workers), on_proxy_connection); // HMM, do I really divide it here?
    if (r != 0) printf("Error: %s\n", uv_err_name(r));

    r = uv_run(listener->loop, UV_RUN_DEFAULT);
    if (r != 0) printf("Error: %s\n", uv_err_name(r));
}

int main() {
    setbuf(stdout, 0);
    srand(time(NULL));

    init_listeners();
    setup_servers_map();
    setup_player_map();

    load_settings();
    load_keys();
    init_ssl();

    load_plugins("plugins");

    uv_loop_t *loop = uv_default_loop();

    int r;
    uv_tcp_t server;
    multiplex_t multiplex;
    uv_tcp_init(loop, &server);

    struct sockaddr_in addr;
    uv_ip4_addr(settings.address, settings.port, &addr);
    uv_tcp_bind(&server, (const struct sockaddr *) &addr, 0);

    init_worker_context(settings.workers);
    r = multiplex_init(&multiplex, &server, PIPE_NAME, settings.workers, on_worker_start);
    if (r != 0) {
        fprintf(stderr, "Init error %s\n", uv_err_name(r));
        exit(-1);
    }

    unsigned int i;
    for (i = 0; i < settings.workers; i++) {
        worker_context *worker_context = worker_context_get(i);
        worker_context->id = (int) i;

        init_ssl_ctx(worker_context);
        r = multiplex_worker_create(&multiplex, i, worker_context);
        if (r != 0) {
            fprintf(stderr, "Worker error %s\n", uv_err_name(r));
            exit(-1);
        }

        worker_context->loop = &multiplex.workers[i].loop;
        worker_context->players = linked_list_new();
    }

    r = multiplex_dispatch(&multiplex);
    if (r != 0) {
        fprintf(stderr, "Dispatch error %s\n", r == 1 ? "My error" : uv_err_name(r));
        exit(-1);
    }

    printf("[Proxy] Listening on port %d.\n", settings.port);
    puts("[Proxy] Enter any character to exit.");

    getchar();

    // Disable plugins
    // Free everything (or is this necessary?)

    return 0;
}
