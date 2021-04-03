#ifndef PHOTON_CONVERSION_H
#define PHOTON_CONVERSION_H
#include <stdbool.h>
#include <stdint.h>

int read_int(const char *buf, int *index);
char* read_str(const char *buf, int *index, int size);
char* read_str_b(const char *buf, int *index, int size, char *str);
char* read_var_str(const char *buf, int *index);
char* read_var_str_s(const char *buf, int *index, int *size);
unsigned short read_u_short(const char *buf, int *index);
short read_short(const char *buf, int *index);
int read_var_int(const char *buf, int *index);
int64_t read_long(const char *buf, int *index);
char *read_bytes(const char*buf, int *index, int size);
unsigned char *read_u_bytes(const char*buf, int *index, int size);
unsigned char *read_u_bytes_b(const char*buf, int *index, int size, unsigned char *str);
bool read_bool(const char *buf, int *index);
unsigned char read_u_byte(const char *buf, int *index);
char read_byte(const char *buf, int *index);

int get_var_int_size(int num);
void write_var_int(char *buf, int *index, int num);
void write_str(char *buf, int *index, int size, char *str);
void write_bytes(char *buf, int *index, int size, char *data);
void write_byte(char *buf, int *index, char byte);
void write_u_byte(char *buf, int *index, unsigned char byte);
void write_long(char *buf, int *index, int64_t num);
void write_int(char *buf, int *index, int32_t num);
void write_u_short(char *buf, int *index, unsigned short num);
void write_float(char *buf, int *index, float num);

char *encode_base64(char *source, size_t source_length);

// Compress and decompress
char *photon_uncompress(char *source, long source_length, long output_length);
void photon_compress(int source_length, char *source, int *output_size, char* output);

char *read_utf(char *data, int *index);
void skip_s_tag(char *data, int *index, char type);
char skip_tag(char *data, int *index);

#endif //PHOTON_CONVERSION_H
