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
output->x = *(double*) htobe64(*(uint64_t*) &input->x);
output->y = *(double*) htobe64(*(uint64_t*) &input->y);
output->z = *(double*) htobe64(*(uint64_t*) &input->z);
}

void encode_distance(struct distance* input, struct distance* output) {
output->x = *(double*) htobe64(*(uint64_t*) &input->x);
output->y = *(double*) htobe64(*(uint64_t*) &input->y);
output->z = *(double*) htobe64(*(uint64_t*) &input->z);
}

void decode_position(struct position* input, struct position* output) {
output->x = *(double*) be64toh(*(uint64_t*) &input->x);
output->y = *(double*) be64toh(*(uint64_t*) &input->y);
output->z = *(double*) be64toh(*(uint64_t*) &input->z);
}

void decode_distance(struct distance* input, struct distance* output) {
output->x = *(double*) be64toh(*(uint64_t*) &input->x);
output->y = *(double*) be64toh(*(uint64_t*) &input->y);
output->z = *(double*) be64toh(*(uint64_t*) &input->z);
}
