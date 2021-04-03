#ifndef PHOTON_SETTINGS_H
#define PHOTON_SETTINGS_H

#include <stdbool.h>

#include "util/types.h"

typedef struct photon_settings_s {
    int port;
    char address[17];
    unsigned int max_connections;
    unsigned int workers;
    char bridge_password[256];
    bool prevent_proxy_connections;
    mc_server *fallback_server;
    mc_server *main_server;
    unsigned int compression_threshold;

    // Status
    char *status_sample;
    char *status_description;
    char *status_name;
    int status_protocol;
    char *status_favicon;
} photon_settings;

void load_settings();
void load_servers();

#endif //PHOTON_SETTINGS_H
