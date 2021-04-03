#include "uv.h"

#include "session.h"
#include "player/player.h"

photon_settings settings = {
        25565,
        "0.0.0.0",
        5000,
        2,
        "thisisagreatpassword",
        true,
        NULL,
        NULL,
        256,
        "{ \"name\": \"Photon server!\", \"id\": \"7c8896e2-8c7b-45ef-bb08-8c8e64528d0a\" }",
        "A photon proxy.",
        "1.16.5",
        754,
        "<data>"
};

uv_rwlock_t players_lock;
hashmap *username_players;
hashmap *uuid_players;

void *username_from_player(void *value) {
    return ((player*) value)->username;
}

void *uuid_from_player(void *value) {
    return &((player*) value)->uuid;
}

unsigned int get_player_count() {
    return hashmap_size(username_players);
}

player *get_player_s(char *username) {
    player *player = hashmap_get(username_players, username);
    return player;
}

player *get_player_u(uuid *uuid) {
    player *player = hashmap_get(uuid_players, uuid);
    return player;
}

void add_player(player *player) {
    hashmap_put(username_players, player->username, player);
    hashmap_put(uuid_players, &player->uuid, player);
}

void remove_player(player *player) {
    hashmap_remove(username_players, player->username);
    hashmap_remove(uuid_players, &player->uuid);
}

void setup_player_map() {
    username_players = hashmap_new(16 * 2, str_hash, str_compare, username_from_player);
    uuid_players = hashmap_new(16 * 2, uuid_hash, uuid_compare, uuid_from_player);

    uv_rwlock_init(&players_lock);
}

void rlock_players() {
    uv_rwlock_rdlock(&players_lock);
}
void wlock_players() {
    uv_rwlock_wrlock(&players_lock);
}

void runlock_players() {
    uv_rwlock_rdunlock(&players_lock);
}

void wunlock_players() {
    uv_rwlock_wrunlock(&players_lock);
}