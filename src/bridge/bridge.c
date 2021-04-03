#include "bridge.h"

#include "phapi/phapi.h"

void on_bridge_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    proxy_context *ctx = client->data;
    if (nread < 0) {
        if (nread != UV_EOF)
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));

        uv_close((uv_handle_t *) client, on_close);
        free(ctx);
        return;
    }

    event_bridge_message event_message = { ctx };

    int index = 0;
    while (index < nread) {
        unsigned char packet_name_length = read_u_byte(buf->base, &index);
        if (packet_name_length == 0) break;

        char packet_name[30];
        read_str_b(buf->base, &index, packet_name_length, packet_name);
        int packet_size = read_int(buf->base, &index);

        printf("-> %d %s %d\n", packet_name_length, packet_name, packet_size);

        event_message.packet_name_size = packet_name_length;
        event_message.packet_name = packet_name;
        event_message.data = buf->base + index;
        event_message.nread = packet_size;
        run_event(EVENT_BRIDGE_MESSAGE, &event_message);

        index += packet_size;
    }
}
