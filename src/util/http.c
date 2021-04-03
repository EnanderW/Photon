#include "util/http.h"

#include <openssl/aes.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <uv.h>
#include <stdlib.h>
#include <openssl/conf.h>
#include <string.h>

#include "proxy/connectionrequest.h"
#include "proxy/proxycontext.h"

void init_ssl() {
    SSL_library_init();
    SSL_load_error_strings();
    ERR_load_BIO_strings();
    OpenSSL_add_all_algorithms();
    OPENSSL_config(NULL);
}

void init_ssl_ctx(worker_context* ctx) {
    ctx->ssl_ctx = SSL_CTX_new(SSLv23_client_method());
    SSL_CTX_set_options(ctx->ssl_ctx, SSL_OP_NO_SSLv2);
}

void on_written_callback(uv_write_t* req, int status);

typedef struct {
    uv_tcp_t socket;
    uv_write_t write_req;
    uv_connect_t connect_req;
    char host[1024];
    char port[1024];
    char page[1024];
    char *buffer_out;
    SSL_CTX* ssl_ctx;
    SSL* ssl;
    BIO* read_bio;
    BIO* write_bio;
    connection_request *photon_request;
    void (*callback)(connection_request *request, char*);
} ssl_client;

void client_free(ssl_client *client) {
    if (client->ssl)
        SSL_free(client->ssl);

    free(client);
}

static void on_close(uv_handle_t *handle) {
    ssl_client* client = handle->data;
    client_free(client);
}

bool compare_response(char *response, char *expected, int size) {
    for (int i = 0; i < size; i++) {
        if (response[i] != expected[i]) return false;
    }

    return true;
}

char* get_json_content(char *text) {
    char *r_code = strstr(text, " ") + 1;
    if (!compare_response(r_code, "200", 3)) return NULL;

    return strstr((text + 350), "\r\n\r\n") + 4; // Sneaky way to get json part
}

void write_to_socket(ssl_client* client, char* buf, size_t len) {
    if(len <= 0) return;

    uv_buf_t uvbuf = uv_buf_init(buf, len);
    int r = uv_write(&client->write_req, (uv_stream_t*)&client->socket, &uvbuf, 1, on_written_callback);
    if(r < 0) {
        printf("ERROR: write_to_socket error: %s\n", uv_err_name(len));
    }
}

void flush_read_bio(ssl_client* client) {
    char buf[1024*16];
    int bytes_read = 0;
    while((bytes_read = BIO_read(client->write_bio, buf, 1024 * 16)) > 0) {
        write_to_socket(client, buf, bytes_read);
    }
}

void handle_error(ssl_client* client, int result) {
    int error = SSL_get_error(client->ssl, result);
    if(error == SSL_ERROR_WANT_READ) { // wants to read from bio
        flush_read_bio(client);
    }
}

void check_outgoing_application_data(ssl_client* client) {
    if(SSL_is_init_finished(client->ssl)) {
        int r = SSL_write(client->ssl, client->buffer_out, strlen(client->buffer_out));
        handle_error(client, r);
        flush_read_bio(client);
    }
}

void on_event(ssl_client* client, int bytes_read) {
    char buf[1024 * 10];

    if(!SSL_is_init_finished(client->ssl)) {
        int r = SSL_connect(client->ssl);
        if(r < 0) {
            handle_error(client, r);
            return;
        }

        check_outgoing_application_data(client);
    } else {
        int r = SSL_read(client->ssl, buf, bytes_read);
        if (r < 0) {
            handle_error(client, r);
        } else if(r > 0) {
            char *json = get_json_content(buf);
            client->callback(client->photon_request, json);
        }
    }
}

void on_alloc_callback(uv_handle_t *handle, size_t size, uv_buf_t *buf) {
    ssl_client *client = handle->data;
    worker_context *worker_context = client->photon_request->proxy_context->worker_context;
    buf->base = worker_context->in_data;
    buf->len = sizeof(worker_context->in_data);
}

void on_written_callback(uv_write_t* req, int status) { }

void on_read_callback(uv_stream_t* tcp, ssize_t nread, const uv_buf_t *buf) {
    ssl_client* client = tcp->data;
    if(nread < 0) {
        if (nread != UV_EOF) {
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));
            client->callback(client->photon_request, NULL);
        }

        uv_close((uv_handle_t *) tcp, on_close);
        return;
    }

    int written = BIO_write(client->read_bio, buf->base, nread);
    on_event(client, nread);
}

void on_connect_callback(uv_connect_t* con, int status) {
    ssl_client* client = con->data;
    if(status < 0) {
        printf("ERROR: on_connect_callback %s\n", uv_err_name(status));
        client_free(client);
        free(con);
        return;
    }

    int r = uv_read_start((uv_stream_t*) &(client->socket), on_alloc_callback, on_read_callback);
    if(r == -1) {
        printf("ERROR: uv_read_start error: %s\n", uv_err_name(status));
        return;
    }

    const char* http_request_tpl = "GET %s HTTP/1.1\r\n"
                                   "Host: %s\r\n"
                                   "Connection: close\r\n"
                                   "\r\n";

    char http_request[2048];
    sprintf(http_request, http_request_tpl, client->page, client->host);

    memcpy(client->buffer_out, http_request, strlen(http_request));

    client->ssl = SSL_new(client->ssl_ctx);
    client->read_bio = BIO_new(BIO_s_mem());
    client->write_bio = BIO_new(BIO_s_mem());
    SSL_set_bio(client->ssl, client->read_bio, client->write_bio);
    SSL_set_connect_state(client->ssl);

    r = SSL_do_handshake(client->ssl);
    on_event(client, 0);
}

void on_resolved_callback(uv_getaddrinfo_t* resolver, int status, struct addrinfo * res) {
    ssl_client* client = resolver->data;
    if(status != 0) {
        printf("ERROR: getaddrinfo callback error: %s\n", uv_err_name(status));
        client_free(client);
        uv_freeaddrinfo(res);
        free(resolver);
        return;
    }

    char addr[17] = { 0 };
    uv_ip4_name((struct sockaddr_in*) res->ai_addr, addr, 16);

    client->socket.data = client;
    uv_tcp_init(resolver->loop, &(client->socket));
    uv_tcp_connect(&client->connect_req, &client->socket, res->ai_addr, on_connect_callback);

    uv_freeaddrinfo(res);
    free(resolver);
}

int send_request(connection_request *request, char *params, void (*callback)(connection_request *request, char* json)) {
    // Client context
    ssl_client *client = malloc(sizeof(ssl_client));
    client->connect_req.data = client;
    client->socket.data = client;
    client->ssl = NULL;
    client->ssl_ctx = request->proxy_context->worker_context->ssl_ctx;
    client->photon_request = request;
    client->callback = callback;

    worker_context *worker_context = client->photon_request->proxy_context->worker_context;
    client->buffer_out = worker_context->out_data;

    sprintf(client->host, "%s", "sessionserver.mojang.com");
    sprintf(client->port, "%s", "443");
    sprintf(client->page, "%s?%s", "/session/minecraft/hasJoined", params);

    // Resolve host
    struct addrinfo hints;
    hints.ai_family = PF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = 0;
    uv_getaddrinfo_t *resolver = malloc(sizeof(uv_getaddrinfo_t));
    resolver->data = client;
    int r = uv_getaddrinfo(request->player_client->loop, resolver, on_resolved_callback, client->host, client->port, &hints);
    return r;
}