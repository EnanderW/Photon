#ifndef PHOTON_BRIDGE_H
#define PHOTON_BRIDGE_H

#include <uv.h>

void on_bridge_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf);

#endif //PHOTON_BRIDGE_H
