#ifndef PHOTON_PROXYCONTEXT_H
#define PHOTON_PROXYCONTEXT_H

#include "workercontext.h"

typedef struct proxy_context_s {
    worker_context *worker_context;
    void *handle;
} proxy_context;

#endif //PHOTON_PROXYCONTEXT_H
