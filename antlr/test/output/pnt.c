#include <endian.h>
#include <stdint.h>


struct position {
double x __attribute__((aligned(8)));
double y __attribute__((aligned(8)));
double z __attribute__((aligned(8)));
};

struct distance {
double x __attribute__((aligned(8)));
double y __attribute__((aligned(8)));
double z __attribute__((aligned(8)));
};

void encode_position(struct position* input, struct position* output) {
uint64_t x = *(uint64_t*) &input->x;
uint64_t y = *(uint64_t*) &input->y;
uint64_t z = *(uint64_t*) &input->z;
x = htobe64(x);
y = htobe64(y);
z = htobe64(z);
output->x = *(double*) &x;
output->y = *(double*) &y;
output->z = *(double*) &z;
}

void encode_distance(struct distance* input, struct distance* output) {
uint64_t x = *(uint64_t*) &input->x;
uint64_t y = *(uint64_t*) &input->y;
uint64_t z = *(uint64_t*) &input->z;
x = htobe64(x);
y = htobe64(y);
z = htobe64(z);
output->x = *(double*) &x;
output->y = *(double*) &y;
output->z = *(double*) &z;
}

void decode_position(struct position* input, struct position* output) {
uint64_t x = *(uint64_t*) &input->x;
uint64_t y = *(uint64_t*) &input->y;
uint64_t z = *(uint64_t*) &input->z;
x = be64toh(x);
y = be64toh(y);
z = be64toh(z);
output->x = *(double*) &x;
output->y = *(double*) &y;
output->z = *(double*) &z;
}

void decode_distance(struct distance* input, struct distance* output) {
uint64_t x = *(uint64_t*) &input->x;
uint64_t y = *(uint64_t*) &input->y;
uint64_t z = *(uint64_t*) &input->z;
x = be64toh(x);
y = be64toh(y);
z = be64toh(z);
output->x = *(double*) &x;
output->y = *(double*) &y;
output->z = *(double*) &z;
}
