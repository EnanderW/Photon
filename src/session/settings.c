#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "settings.h"
#include "session.h"
#include "mc_server.h"

#define ADDRESS if (compare_key(buffer, "address", 7))
#define PORT else if (compare_key(buffer, "port", 4))
#define FALLBACK_SERVER else if (compare_key(buffer, "fallback-server", 15))
#define MAIN_SERVER else if (compare_key(buffer, "main-server", 11))
#define MAX_PLAYERS else if (compare_key(buffer, "max-players", 11))
#define COMPRESSION_THRESHOLD else if (compare_key(buffer, "compression-threshold", 21))
#define PREVENT_PROXY_CONNECTIONS else if (compare_key(buffer, "prevent-proxy-connections", 25))
#define SERVER_ICON else if (compare_key(buffer, "server-icon", 11))
#define STATUS_NAME else if (compare_key(buffer, "status-name", 11))
#define STATUS_SAMPLE else if (compare_key(buffer, "status-sample", 13))
#define STATUS_DESCRIPTION else if (compare_key(buffer, "status-description", 18))
#define STATUS_PROTOCOL else if (compare_key(buffer, "status-protocol", 15))

static bool compare_key(char *key, char *actual, int length) {
    for (int i = 0; i < length; i++)
        if (key[i] != actual[i]) return false;

    return true;
}

static char *get_space(char *buffer) {
    char *space = strstr(buffer, " ");
    if (space == NULL) {
        puts("Error during reading servers.set file.");
        exit(-1);
    }

    return space + 1;
}

static void create_settings_file(const char *path) {
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        printf("An error occurred during the creation of the config file.");
        exit(-1);
    }

    fputs("address: 0.0.0.0\n", file);
    fputs( "port: 25565\n", file);
    fputs("fallback-server: lobby\n", file);
    fputs("main-server: lobby\n", file);
    fputs("max-players: 5000\n", file);
    fputs("compression-threshold: 256\n", file);
    fputs("prevent-proxy-connections: false\n", file);
    fputs("server-icon: server-icon.png\n", file);
    fputs("status-sample: { \"name\": \"Photon server!\", \"id\": \"7c8896e2-8c7b-45ef-bb08-8c8e64528d0a\" }\n", file);
    fputs("status-name: 1.16.5\n", file);
    fputs("status-description: A photon proxy.\n", file);
    fputs("status-protocol: 754\n", file);

    fclose(file);
}

static void create_servers_file(const char *path) {
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        printf("An error occurred during the creation of the servers file.");
        exit(-1);
    }

    fputs("lobby 127.0.0.1 25566\n", file);
    fputs("other_server 127.0.0.1 25567\n", file);
    fclose(file);
}

#include "util/conversion.h"
static void load_config(FILE *file) {
    char bufs[1000];
    char *buffer = bufs;
    while (true) {
        char *r = fgets(buffer, 200, file);
        if (r == NULL) break;

        char *value_start = strstr(buffer, ": ");
        if (value_start == NULL) continue;

        char value[50] = {0};
        strcpy(value,value_start + 2);
        *(value_start) = 0;
        *(value + strlen(value) - 1) = 0;

        ADDRESS {
            strcpy(settings.address, value);
        }
        PORT {
            settings.port = atoi(value);
        }
        FALLBACK_SERVER {
            settings.fallback_server = get_server(value);
        }
        MAIN_SERVER {
            settings.main_server = get_server(value);
        }
        COMPRESSION_THRESHOLD {
            settings.compression_threshold = atoi(value);
        }
        PREVENT_PROXY_CONNECTIONS {
            settings.prevent_proxy_connections = strcmp("true", value) ? true : false;
        }
        MAX_PLAYERS {
            settings.max_connections = atoi(value);
        }
        SERVER_ICON {
            char *file_name = value;
            FILE *icon = fopen(file_name, "rb");
            if (icon == NULL) {
                printf("Could not find server icon file.\n");
            } else {
                fseek(icon, 0, SEEK_END);
                int icon_size = ftell(icon);
                fseek(icon, 0, SEEK_SET);

                char *buf = malloc(icon_size);
                fread(buf, icon_size, 1, icon);

                char *data = encode_base64(buf, icon_size);

                settings.status_favicon = data;

                free(buf);

                fclose(icon);
            }
        }
        STATUS_NAME {
            settings.status_name = malloc(strlen(value) + 1);
            strcpy(settings.status_name, value);
        }
        STATUS_DESCRIPTION {
            settings.status_description = malloc(strlen(value) + 1);
            strcpy(settings.status_description, value);
        }
        STATUS_PROTOCOL {
            settings.status_protocol = atoi(value);
        }
        STATUS_SAMPLE {
            settings.status_sample = malloc(strlen(value) + 1);
            strcpy(settings.status_sample, value);
        }
    }

    fclose(file);
}

void load_servers(FILE *file) {
    char bufs[100];
    char *buffer = bufs;
    while (true) {
        char *r = fgets(buffer, 100, file);
        if (r == NULL) break;

        char *server = buffer;
        char *address = get_space(buffer);
        char *port = get_space(address);

        *(port - 1) = 0;
        *(address - 1) = 0;

        add_server(server, address, atoi(port));
    }

    fclose(file);
}

void load_settings() {
    bool any = false;
    FILE *config = fopen("config.set", "r");
    if (config == NULL) {
        create_settings_file("config.set");
        any = true;
    }

    FILE *servers = fopen("servers.set", "r");
    if (servers == NULL) {
        create_servers_file("servers.set");
        any = true;
    }

    if (any) {
        printf("Created config and servers configuration files.");
        exit(0);
    }

    load_servers(servers);
    load_config(config);
}