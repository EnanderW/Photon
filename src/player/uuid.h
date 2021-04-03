#ifndef PHOTON_UUID_H
#define PHOTON_UUID_H

#include <stdint.h>

typedef struct uuid_s {
    uint64_t most;
    uint64_t least;
} uuid;

#endif //PHOTON_UUID_H
