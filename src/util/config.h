#ifndef PHOTON_CONFIG_H
#define PHOTON_CONFIG_H

#include <uv.h>
typedef struct evp_cipher_ctx_st EVP_CIPHER_CTX;

void on_write(uv_write_t *req, int status);
void on_close(uv_handle_t *handle);

struct proxy_keys {
    int pub_key_size;
    int pri_key_size;
    char *pub_key;
    char *pri_key;

    void* rsa;
};

extern struct proxy_keys keys;

void load_keys();
void rsa_decrypt(unsigned char *to, unsigned char *from, int size);
void aes_decrypt(EVP_CIPHER_CTX *ctx, unsigned char *cipher_data, unsigned char *out, int *length);
void aes_encrypt(EVP_CIPHER_CTX *ctx, unsigned char *plain_data, unsigned char *out, int *length);

#endif //PHOTON_CONFIG_H
