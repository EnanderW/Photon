#include <malloc.h>
#include <string.h>

#include "session/session.h"
#include "phapi/phapi.h"
#include "stage/playerstage.h"
#include "proxy/connectionrequest.h"

void send_server_list_ping(connection_request *request);
void send_pong(connection_request *request, int64_t payload);

void player_status_stage(connection_request *request, char *data, int *index, ssize_t nread) {
    int packet_length = read_var_int(data, index);
    int packet_id = read_var_int(data, index);

    if (packet_id == 0) {
        send_server_list_ping(request);
    } else if (packet_id == 1) {
        int64_t payload = read_long(data, index);
        send_pong(request, payload);
    } else {
        uv_close((uv_handle_t *) request->player_client, on_close);
        free(request->proxy_context);
        connection_request_free_full(request);
        *index = nread;
    }
}

void send_pong(connection_request *request, int64_t payload) {
    int packet_id = 0x01;
    int packet_length = get_var_int_size(packet_id) + sizeof(int64_t);
    int packet_size = get_var_int_size(packet_length) + packet_length;

    char *pack = malloc(sizeof(char) * packet_size);
    int index = 0;

    write_var_int(pack, &index, packet_length);
    write_var_int(pack, &index, packet_id);
    write_long(pack, &index, payload);

    uv_write_t *req = malloc(sizeof(uv_write_t));
    uv_buf_t wrbuf = uv_buf_init(pack, packet_size);
    uv_write(req, request->player_client, &wrbuf, 1, on_write);

    free(pack);
}

void send_server_list_ping(connection_request *request) {
    rlock_players();
    unsigned int players_s = get_player_count();
    runlock_players();

    event_status_ping event_ping = { (int) settings.max_connections, (int) players_s, settings.status_sample, settings.status_name, settings.status_protocol, settings.status_description, settings.status_favicon };
    run_event(EVENT_STATUS_PING, &event_ping);

    char *template = "{ \"version\": { \"name\": \"%s\", \"protocol\": %d }, \"players\": { \"max\": %d, \"online\": %d, \"sample\": [ %s ] }, \"description\": \"%s\", \"favicon\": \"data:image/png;base64,%s\" }";

    char json[20000];
    sprintf(json, template, event_ping.name, event_ping.protocol, event_ping.max_players, event_ping.players, event_ping.sample, event_ping.description, event_ping.favicon);
    int json_length = strlen(json);

    int packet_id = 0x00;
    int packet_length = get_var_int_size(packet_id) + get_var_int_size(json_length) + json_length;
    int packet_size = get_var_int_size(packet_length) + packet_length;

    char *pack = malloc(sizeof(char) * packet_size);
    int index = 0;

    write_var_int(pack, &index, packet_length);
    write_var_int(pack, &index, packet_id);
    write_str(pack, &index, json_length, json);

    uv_write_t *req = malloc(sizeof(uv_write_t));
    uv_buf_t wrbuf = uv_buf_init(pack, packet_size);
    uv_write(req, request->player_client, &wrbuf, 1, on_write);

    free(pack);
}
