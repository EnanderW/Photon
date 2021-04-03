#include "config.h"
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <stdio.h>
#include <openssl/x509.h>

struct proxy_keys keys;

void on_write(uv_write_t *req, int status) {
    if (status) {
        fprintf(stderr, "Write error %s\n", uv_strerror(status));
    }

    free(req);
}

void on_close(uv_handle_t *handle) {
    free(handle);
}

void load_keys() {
    int ret = 0;
    BIGNUM *exp = NULL;

    exp = BN_new();
    ret = BN_set_word(exp, RSA_F4);
    if (ret != 1) {
        printf("Error 1?\n");
        return;
    }

    RSA *rsa = RSA_new();
    ret = RSA_generate_key_ex(rsa, 1024, exp, NULL);
    if (ret != 1) {
        printf("Error 2?\n");
        return;
    }

    BIO *der_pub = BIO_new(BIO_s_mem());
    ret = i2d_RSA_PUBKEY_bio(der_pub, rsa);
    if (ret != 1) {
        printf("Error 3?\n");
        return;
    }

    BIO *der_pri = BIO_new(BIO_s_mem());
    ret = i2d_RSAPrivateKey_bio(der_pri, rsa);
    if (ret != 1) {
        printf("Error 4?\n");
        return;
    }

    int pub_key_size = BIO_pending(der_pub);
    char *pub_key = malloc(pub_key_size);

    int pri_key_size = BIO_pending(der_pri);
    char *pri_key = malloc(pri_key_size);

    BIO_read(der_pub, pub_key, pub_key_size);
    BIO_read(der_pri, pri_key, pri_key_size);

    keys.pub_key = pub_key;
    keys.pri_key = pri_key;
    keys.pub_key_size = pub_key_size;
    keys.pri_key_size = pri_key_size;
    keys.rsa = rsa;

    BN_free(exp);
    BIO_free(der_pub);
    BIO_free(der_pri);
}

void rsa_decrypt(unsigned char *to, unsigned char *from, int size) {
    RSA_private_decrypt(size, from, to, keys.rsa, RSA_PKCS1_PADDING);
}

void aes_decrypt(EVP_CIPHER_CTX *ctx, unsigned char *cipher_data, unsigned char *out, int *length) {
    EVP_DecryptUpdate(ctx, out, length, cipher_data, *length);
}

void aes_encrypt(EVP_CIPHER_CTX *ctx, unsigned char *plain_data, unsigned char *out, int *length) {
    EVP_EncryptUpdate(ctx, out, length, plain_data, *length);
}