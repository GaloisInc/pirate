#include <endian.h>
#include <stdint.h>


struct primitives {
float float_val __attribute__((aligned(4)));
double double_val __attribute__((aligned(8)));
int16_t short_val __attribute__((aligned(2)));
int32_t long_val __attribute__((aligned(4)));
int64_t long_long_val __attribute__((aligned(8)));
uint16_t unsigned_short_val __attribute__((aligned(2)));
uint32_t unsigned_long_val __attribute__((aligned(4)));
uint64_t unsigned_long_long_val __attribute__((aligned(8)));
char char_val __attribute__((aligned(1)));
uint8_t bool_val __attribute__((aligned(1)));
uint8_t octet_val __attribute__((aligned(1)));
};

struct alt_primitives {
int8_t int8_val __attribute__((aligned(1)));
int16_t int16_val __attribute__((aligned(2)));
int32_t int32_val __attribute__((aligned(4)));
int64_t int64_val __attribute__((aligned(8)));
uint8_t uint8_val __attribute__((aligned(1)));
uint16_t uint16_val __attribute__((aligned(2)));
uint32_t uint32_val __attribute__((aligned(4)));
uint64_t uint64_val __attribute__((aligned(8)));
};

void encode_primitives(struct primitives* input, struct primitives* output) {
uint32_t float_val = *(uint32_t*) &input->float_val;
uint64_t double_val = *(uint64_t*) &input->double_val;
uint16_t short_val = *(uint16_t*) &input->short_val;
uint32_t long_val = *(uint32_t*) &input->long_val;
uint64_t long_long_val = *(uint64_t*) &input->long_long_val;
uint16_t unsigned_short_val = *(uint16_t*) &input->unsigned_short_val;
uint32_t unsigned_long_val = *(uint32_t*) &input->unsigned_long_val;
uint64_t unsigned_long_long_val = *(uint64_t*) &input->unsigned_long_long_val;
float_val = htobe32(float_val);
double_val = htobe64(double_val);
short_val = htobe16(short_val);
long_val = htobe32(long_val);
long_long_val = htobe64(long_long_val);
unsigned_short_val = htobe16(unsigned_short_val);
unsigned_long_val = htobe32(unsigned_long_val);
unsigned_long_long_val = htobe64(unsigned_long_long_val);
output->float_val = *(float*) &float_val;
output->double_val = *(double*) &double_val;
output->short_val = *(int16_t*) &short_val;
output->long_val = *(int32_t*) &long_val;
output->long_long_val = *(int64_t*) &long_long_val;
output->unsigned_short_val = *(uint16_t*) &unsigned_short_val;
output->unsigned_long_val = *(uint32_t*) &unsigned_long_val;
output->unsigned_long_long_val = *(uint64_t*) &unsigned_long_long_val;
output->char_val = input->char_val;
output->bool_val = input->bool_val;
output->octet_val = input->octet_val;
}

void encode_alt_primitives(struct alt_primitives* input, struct alt_primitives* output) {
uint16_t int16_val = *(uint16_t*) &input->int16_val;
uint32_t int32_val = *(uint32_t*) &input->int32_val;
uint64_t int64_val = *(uint64_t*) &input->int64_val;
uint16_t uint16_val = *(uint16_t*) &input->uint16_val;
uint32_t uint32_val = *(uint32_t*) &input->uint32_val;
uint64_t uint64_val = *(uint64_t*) &input->uint64_val;
int16_val = htobe16(int16_val);
int32_val = htobe32(int32_val);
int64_val = htobe64(int64_val);
uint16_val = htobe16(uint16_val);
uint32_val = htobe32(uint32_val);
uint64_val = htobe64(uint64_val);
output->int8_val = input->int8_val;
output->int16_val = *(int16_t*) &int16_val;
output->int32_val = *(int32_t*) &int32_val;
output->int64_val = *(int64_t*) &int64_val;
output->uint8_val = input->uint8_val;
output->uint16_val = *(uint16_t*) &uint16_val;
output->uint32_val = *(uint32_t*) &uint32_val;
output->uint64_val = *(uint64_t*) &uint64_val;
}

void decode_primitives(struct primitives* input, struct primitives* output) {
uint32_t float_val = *(uint32_t*) &input->float_val;
uint64_t double_val = *(uint64_t*) &input->double_val;
uint16_t short_val = *(uint16_t*) &input->short_val;
uint32_t long_val = *(uint32_t*) &input->long_val;
uint64_t long_long_val = *(uint64_t*) &input->long_long_val;
uint16_t unsigned_short_val = *(uint16_t*) &input->unsigned_short_val;
uint32_t unsigned_long_val = *(uint32_t*) &input->unsigned_long_val;
uint64_t unsigned_long_long_val = *(uint64_t*) &input->unsigned_long_long_val;
float_val = be32toh(float_val);
double_val = be64toh(double_val);
short_val = be16toh(short_val);
long_val = be32toh(long_val);
long_long_val = be64toh(long_long_val);
unsigned_short_val = be16toh(unsigned_short_val);
unsigned_long_val = be32toh(unsigned_long_val);
unsigned_long_long_val = be64toh(unsigned_long_long_val);
output->float_val = *(float*) &float_val;
output->double_val = *(double*) &double_val;
output->short_val = *(int16_t*) &short_val;
output->long_val = *(int32_t*) &long_val;
output->long_long_val = *(int64_t*) &long_long_val;
output->unsigned_short_val = *(uint16_t*) &unsigned_short_val;
output->unsigned_long_val = *(uint32_t*) &unsigned_long_val;
output->unsigned_long_long_val = *(uint64_t*) &unsigned_long_long_val;
output->char_val = input->char_val;
output->bool_val = input->bool_val;
output->octet_val = input->octet_val;
}

void decode_alt_primitives(struct alt_primitives* input, struct alt_primitives* output) {
uint16_t int16_val = *(uint16_t*) &input->int16_val;
uint32_t int32_val = *(uint32_t*) &input->int32_val;
uint64_t int64_val = *(uint64_t*) &input->int64_val;
uint16_t uint16_val = *(uint16_t*) &input->uint16_val;
uint32_t uint32_val = *(uint32_t*) &input->uint32_val;
uint64_t uint64_val = *(uint64_t*) &input->uint64_val;
int16_val = be16toh(int16_val);
int32_val = be32toh(int32_val);
int64_val = be64toh(int64_val);
uint16_val = be16toh(uint16_val);
uint32_val = be32toh(uint32_val);
uint64_val = be64toh(uint64_val);
output->int8_val = input->int8_val;
output->int16_val = *(int16_t*) &int16_val;
output->int32_val = *(int32_t*) &int32_val;
output->int64_val = *(int64_t*) &int64_val;
output->uint8_val = input->uint8_val;
output->uint16_val = *(uint16_t*) &uint16_val;
output->uint32_val = *(uint32_t*) &uint32_val;
output->uint64_val = *(uint64_t*) &uint64_val;
}
