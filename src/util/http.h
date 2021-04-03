#ifndef PHOTON_HTTP_H
#define PHOTON_HTTP_H

typedef struct connection_request_s connection_request;
typedef struct worker_context_s worker_context;

void init_ssl();
void init_ssl_ctx(worker_context* ctx);
int send_request(connection_request *request, char *params, void (*callback)(connection_request *request, char*));

#endif //PHOTON_HTTP_H
