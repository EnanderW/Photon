#ifndef PHOTON_PLAYERSTAGE_H
#define PHOTON_PLAYERSTAGE_H

#include "stage/stage.h"

void player_handshake_stage(connection_request *request, char *data, int *index, ssize_t nread);
void player_login_stage(connection_request *request, char *data, int *index, ssize_t nread);
void player_play_stage(player *player, char *data, int *index, ssize_t nread);
void player_status_stage(connection_request *request, char *data, int *index, ssize_t nread);

#endif //PHOTON_PLAYERSTAGE_H
