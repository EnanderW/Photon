#ifndef PHOTON_STAGE_H
#define PHOTON_STAGE_H

#include "util/types.h"

typedef long long int ssize_t;

typedef void (*handle_server_stage)(player *player, char *data, int *index, ssize_t nread);
typedef void (*handle_player_stage)(player *player, char *data, int *index, ssize_t nread);
typedef void (*handle_request_stage)(connection_request *request, char *data, int *index, ssize_t nread);
typedef void (*handle_switch_stage)(switch_request *request, char *data, int *index, ssize_t nread);

#endif //PHOTON_STAGE_H
