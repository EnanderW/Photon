#ifndef PHOTON_SESSION_H
#define PHOTON_SESSION_H

#include "util/hashmap.h"
#include "settings.h"

#include "util/types.h"

extern photon_settings settings;

player* get_player_s(char *username);
player* get_player_u(uuid *uuid);
void add_player(player *player);
void remove_player(player *player);
unsigned int get_player_count();
void setup_player_map();

void rlock_players();
void wlock_players();

void runlock_players();
void wunlock_players();

#endif //PHOTON_SESSION_H
