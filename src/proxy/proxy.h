#ifndef PHOTON_PROXY_H
#define PHOTON_PROXY_H

#include <stdio.h>
#include <uv.h>

void alloc_proxy_buffer(uv_handle_t *handle, size_t size, uv_buf_t *buf);
void on_proxy_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf);
void on_initial_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf);
void on_proxy_connection(uv_stream_t *server, int status);

#endif //PHOTON_PROXY_H
