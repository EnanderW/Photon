#ifndef PHOTON_SERVERSTAGE_H
#define PHOTON_SERVERSTAGE_H

#include "stage.h"

// LOGIN
void first_server_login_stage(connection_request *request, char *data, int *index, ssize_t nread);
void first_server_login_stage_compression(connection_request *request, char *data, int *index, ssize_t nread);
void server_login_stage(switch_request *request, char *data, int *index, ssize_t nread);
void server_login_stage_compression(switch_request *request, char *data, int *index, ssize_t nread);

// PLAY
void server_play_stage(player *player, char *data, int *index, ssize_t nread);

#endif //PHOTON_SERVERSTAGE_H
