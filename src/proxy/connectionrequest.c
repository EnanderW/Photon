#include "connectionrequest.h"
#include <openssl/evp.h>

void connection_request_free_full(connection_request *request) {
    if (request->encryption_cipher)
        EVP_CIPHER_CTX_cleanup(request->encryption_cipher);
    if (request->decryption_cipher)
        EVP_CIPHER_CTX_cleanup(request->decryption_cipher);

    free(request);
}

void connection_request_free_partial(connection_request *request) {
    free(request);
}

void switch_request_free(switch_request *request) {
    free(request->proxy_context);
    free(request);
}