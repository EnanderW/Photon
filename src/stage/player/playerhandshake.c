#include <malloc.h>
#include "stage/playerstage.h"

#include "util/config.h"
#include "util/conversion.h"
#include "proxy/connectionrequest.h"

/*
 * Two problems
 * 1. Read too much
 * 2. Read wrong data and operate on it
 *
 */

void player_handshake_stage(connection_request *request, char *data, int *index, ssize_t nread) {
    int packet_length = read_var_int(data, index);
    int packet_id = read_var_int(data, index);

    if (packet_id == 0) {
        int protocol_version = read_var_int(data, index);
        int server_address_length = read_var_int(data, index);
        char server_address[50];
        read_str_b(data, index, server_address_length, server_address);
        unsigned short port = read_u_short(data, index);
        int state = read_var_int(data, index);

        request->player_stage = state == 1 ? player_status_stage : player_login_stage;
    } else {
        uv_close((uv_handle_t *) request->player_client, on_close);
        free(request->proxy_context);
        connection_request_free_full(request);
        *index = nread;
    }
}