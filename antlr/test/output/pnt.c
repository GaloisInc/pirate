#include <endian.h>
#include <stdint.h>
#include <string.h>


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
    uint64_t x;
    uint64_t y;
    uint64_t z;
    memcpy(&x, &input->x, sizeof(uint64_t));
    memcpy(&y, &input->y, sizeof(uint64_t));
    memcpy(&z, &input->z, sizeof(uint64_t));
    x = htobe64(x);
    y = htobe64(y);
    z = htobe64(z);
    memcpy(&output->x, &x, sizeof(uint64_t));
    memcpy(&output->y, &y, sizeof(uint64_t));
    memcpy(&output->z, &z, sizeof(uint64_t));
}

void encode_distance(struct distance* input, struct distance* output) {
    uint64_t x;
    uint64_t y;
    uint64_t z;
    memcpy(&x, &input->x, sizeof(uint64_t));
    memcpy(&y, &input->y, sizeof(uint64_t));
    memcpy(&z, &input->z, sizeof(uint64_t));
    x = htobe64(x);
    y = htobe64(y);
    z = htobe64(z);
    memcpy(&output->x, &x, sizeof(uint64_t));
    memcpy(&output->y, &y, sizeof(uint64_t));
    memcpy(&output->z, &z, sizeof(uint64_t));
}

void decode_position(struct position* input, struct position* output) {
    uint64_t x;
    uint64_t y;
    uint64_t z;
    memcpy(&x, &input->x, sizeof(uint64_t));
    memcpy(&y, &input->y, sizeof(uint64_t));
    memcpy(&z, &input->z, sizeof(uint64_t));
    x = be64toh(x);
    y = be64toh(y);
    z = be64toh(z);
    memcpy(&output->x, &x, sizeof(uint64_t));
    memcpy(&output->y, &y, sizeof(uint64_t));
    memcpy(&output->z, &z, sizeof(uint64_t));
}

void decode_distance(struct distance* input, struct distance* output) {
    uint64_t x;
    uint64_t y;
    uint64_t z;
    memcpy(&x, &input->x, sizeof(uint64_t));
    memcpy(&y, &input->y, sizeof(uint64_t));
    memcpy(&z, &input->z, sizeof(uint64_t));
    x = be64toh(x);
    y = be64toh(y);
    z = be64toh(z);
    memcpy(&output->x, &x, sizeof(uint64_t));
    memcpy(&output->y, &y, sizeof(uint64_t));
    memcpy(&output->z, &z, sizeof(uint64_t));
}
