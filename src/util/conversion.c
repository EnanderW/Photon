#include "conversion.h"
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <zlib.h>

int read_int(const char *buf, int *index) {
    int i = *index;
    *index += 4;
    return (((int) buf[i]) << 24) + (((int) buf[i + 1]) << 16) + (((int) buf[i + 2]) << 8) + (((int) buf[i + 3]));
}

int64_t read_long(const char *buf, int *index) {
    int i = *index;
    *index += 8;
    return (((int64_t) buf[i]) << 56)
    + (((int64_t) buf[i + 1]) << 48)
    + (((int64_t) buf[i + 2]) << 40)
    + (((int64_t) buf[i + 3]) << 32)
    + (((int64_t) buf[i + 4]) << 24)
    + (((int64_t) buf[i + 5]) << 16)
    + (((int64_t) buf[i + 6]) << 8)
    + (((int64_t) buf[i + 7]));
}

int read_var_int(const char *buf, int *index) {
    int num_read = 0;
    int result = 0;
    char read;

    int i = *index;
    do {
        read = buf[i++];
        result |= ((read & 0b01111111) << (7 * num_read));

        num_read++;

        if (num_read > 5) {
            fprintf(stderr, "Error reading varint, too many...\n");
            return result;
        }
    } while ((read & 0b10000000) != 0);

    *index = i;
    return result;
}

char* read_str(const char *buf, int *index, int size) {
    char *str = malloc(sizeof(char) * (size) + sizeof(char));
    int i = *index;
    *index += size;

    for (int s = 0; s < size; s++)
        str[s] = buf[i + s];

    str[size] = 0;
    return str;
}

char* read_str_b(const char *buf, int *index, int size, char *str) {
    int i = *index;
    *index += size;

    for (int s = 0; s < size; s++)
        str[s] = buf[i + s];

    str[size] = 0;
    return str;
}

char* read_var_str(const char *buf, int *index) {
    int size = read_var_int(buf, index);
    char *str = malloc(sizeof(char) * size + sizeof(char));
    int i = *index;
    *index += size;

    for (int s = 0; s < size; s++)
        str[s] = buf[i + s];

    str[size] = 0;
    return str;
}

char* read_var_str_s(const char *buf, int *index, int *size) {
    *size = read_var_int(buf, index);
    char *str = malloc(sizeof(char) * (*size) + sizeof(char));
    int i = *index;
    *index += *size;

    for (int s = 0; s < *size; s++)
        str[s] = buf[i + s];

    str[*size] = 0;
    return str;
}

char *read_bytes(const char *buf, int *index, int size) {
    char *str = malloc(sizeof(char) * size);
    int i = *index;
    *index += size;

    for (int s = 0; s < size; s++)
        str[s] = buf[i + s];

    return str;
}

unsigned char *read_u_bytes(const char *buf, int *index, int size) {
    unsigned char *str = malloc(sizeof(char) * size);
    int i = *index;
    *index += size;

    for (int s = 0; s < size; s++)
        str[s] = (unsigned char) buf[i + s];

    return str;
}

unsigned char *read_u_bytes_b(const char*buf, int *index, int size, unsigned char *str) {
    int i = *index;
    *index += size;

    for (int s = 0; s < size; s++)
        str[s] = (unsigned char) buf[i + s];

    return str;
}
short read_short(const char *buf, int *index) {
    int i = *index;
    *index += 2;
    return (((short) buf[i]) << 8) + (((short) buf[i + 1]));
}

unsigned short read_u_short(const char *buf, int *index) {
    int i = *index;
    *index += 2;
    return (((unsigned short)buf[i]) << 8) + (((unsigned short) buf[i + 1]));
}

bool read_bool(const char *buf, int *index) {
    return buf[(*index)++];
}

unsigned char read_u_byte(const char *buf, int *index) {
    return (unsigned char) buf[(*index)++];
}

char read_byte(const char *buf, int *index) {
    return buf[(*index)++];
}

int get_var_int_size(int num) {
    int count = 0;
    do {
        num = (int)((unsigned int)num >> 7);
        count++;
    } while (num != 0);

    return count;
}

void write_var_int(char *buf, int *index, int num) {
    int i = *index;
    do {
        char temp = (char) (num & 0b01111111);

        num = (int)((unsigned int)num >> 7);

        if (num != 0) {
            temp |= 0b10000000;
        }

        buf[i++] = temp;
    } while (num != 0);

    *index = i;
}

void write_str(char *buf, int *index, int size, char *str) {
    write_var_int(buf, index, size);
    memcpy(buf + *index, str, size);
    (*index) = (*index) + size;
}

void write_bytes(char *buf, int *index, int size, char *data) {
    memcpy(buf + *index, data, size);
    (*index) = (*index) + size;
}

void write_byte(char *buf, int *index, char byte) {
    buf[(*index)++] = byte;
}

void write_u_byte(char *buf, int *index, unsigned char byte) {
    buf[(*index)++] = byte;
}

void write_int(char *buf, int *index, int32_t num) {
    int i = *index;
    buf[i++] = num;
    buf[i++] = num >> 8;
    buf[i++] = num >> 16;
    buf[i++] = num >> 24;
    *index = i;
}

void write_long(char *buf, int *index, int64_t num) {
    int i = *index;
    buf[i++] = num;
    buf[i++] = num >> 8;
    buf[i++] = num >> 16;
    buf[i++] = num >> 24;
    buf[i++] = num >> 32;
    buf[i++] = num >> 40;
    buf[i++] = num >> 48;
    buf[i++] = num >> 56;

    *index = i;
}

void write_u_short(char *buf, int *index, unsigned short num) {
    int i = *index;
    buf[i++] = num;
    buf[i++] = num >> 8;
    *index = i;
}

void write_float(char *buf, int *index, float num) {
    int i = *index;
    memcpy(buf + i, &num, sizeof(float));

    *index = i + sizeof(num);
}

#include <openssl/ssl.h>
char *encode_base64(char *source, size_t source_length) {
    BIO *b64_bio = BIO_new(BIO_f_base64());
    BIO *mem_bio = BIO_new(BIO_s_mem());

    BIO_push(b64_bio, mem_bio);
    BIO_set_flags(b64_bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64_bio, source, source_length);
    BIO_flush(b64_bio);

    BUF_MEM *mem_bio_mem_ptr;
    BIO_get_mem_ptr(mem_bio, &mem_bio_mem_ptr);
    BIO_set_close(mem_bio, BIO_NOCLOSE);

    BIO_free_all(b64_bio);
    BUF_MEM_grow(mem_bio_mem_ptr, (*mem_bio_mem_ptr).length + 1);
    (*mem_bio_mem_ptr).data[(*mem_bio_mem_ptr).length] = '\0';

    return (*mem_bio_mem_ptr).data;
}

char *photon_uncompress(char *source, long source_length, long output_length) {
    char *data = malloc(sizeof(char) * output_length);

    z_stream decomp;
    decomp.zalloc = Z_NULL;
    decomp.zfree = Z_NULL;
    decomp.opaque = Z_NULL;
    decomp.avail_in = source_length;
    decomp.next_in = (Bytef *) source;
    decomp.avail_out = output_length;
    decomp.next_out = (Bytef *) data;

    inflateInit(&decomp);
    inflate(&decomp, Z_NO_FLUSH);
    inflateEnd(&decomp);

    return data;
}

void photon_compress(int source_length, char *source, int *output_size, char *output) {
    z_stream defstream;
    defstream.zalloc = Z_NULL;
    defstream.zfree = Z_NULL;
    defstream.opaque = Z_NULL;
    defstream.avail_in = source_length;
    defstream.next_in = (Bytef *)source;
    defstream.avail_out = output_size;
    defstream.next_out = output;

    deflateInit(&defstream, Z_DEFAULT_COMPRESSION);
    deflate(&defstream, Z_FINISH);
    deflateEnd(&defstream);

    *output_size = (char*)defstream.next_out - output;
}

char *read_utf(char *data, int *index) {
    unsigned short utf_length = read_u_short(data, index);
    char *str = read_str(data, index, utf_length);
    return str;
}

void skip_s_tag(char *data, int *index, char type) {
    switch (type) {
        case 1: {
            char byte = read_byte(data, index);
//            printf("%02x\n", byte);
            break;
        }
        case 2:  {
            short s = read_short(data, index);
//            printf("%d\n", s);
            break;
        }
        case 3: {
            int i = read_int(data, index);
//            printf("%d\n", i);
            break;
        }
        case 4: {
            int64_t l = read_long(data, index);
//            printf("%lld", l);
            break;
        }
        case 5: {
            int i = read_int(data, index);
            float f = *(float*) &i;
//            printf("%f", f);
            break;
        }
        case 6: {
            int64_t l = read_long(data, index);
            double d = *(double*) &l;
//            printf("%f", d);
            break;
        }
        case 7: {
//            puts("");
            int length = read_int(data, index);
            for (int i = 0; i < length; i++) {
                char byte = read_byte(data, index);
//                printf("%d. %02x\n", i, byte);
            }
            break;
        }
        case 8: {
            char *utf = read_utf(data, index);
//            printf("%s\n", utf);
            free(utf);
            break;
        }
        case 9: {
            char item_type = read_byte(data, index);
            int count = read_int(data, index);
            if (item_type == 0 && count > 0) return;
            for (int i = 0; i < count; i++) {
                skip_s_tag(data, index, item_type);
            }
            break;
        }
        case 10: {
//            printf("\n");
            while (true) {
//                printf("  ");
                char type_s = skip_tag(data, index);
                if (type_s == 0) return;
            }

            break;
        }
        case 11: {
            int length = read_int(data, index);
            *index += (length * 4);
            break;
        }
        case 12: {
            int length = read_int(data, index);
            *index += (length * 8);
            break;
        }
    }
}

char skip_tag(char *data, int *index) {
    char type = read_byte(data, index);
    if (type == 0) return type;

    // Read name
    char *name = read_utf(data, index);
    // Read payload
    skip_s_tag(data, index, type);

    free(name);
    return type;
}