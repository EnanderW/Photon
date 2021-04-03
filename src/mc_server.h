#ifndef PHOTON_MC_SERVER_H
#define PHOTON_MC_SERVER_H

#include <uv.h>

typedef struct mc_server_s {
    char name[30];
    struct sockaddr_in server_addr;
    struct sockaddr_in bridge_addr;
} mc_server;

void setup_servers_map();
void add_server(char *name, const char *addr, unsigned short port);
mc_server *get_server(char *name);

void on_mc_bridge_connect(uv_connect_t *conn, int status);
void connect_mc_bridge(mc_server *server, char *buf, int size);


#endif //PHOTON_MC_SERVER_H
