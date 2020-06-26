#include <assert.h>
#include <endian.h>
#include <fenv.h>
#include <math.h>
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

struct position_wire {
	unsigned char x[8] __attribute__((aligned(8)));
	unsigned char y[8] __attribute__((aligned(8)));
	unsigned char z[8] __attribute__((aligned(8)));
};

struct distance_wire {
	unsigned char x[8] __attribute__((aligned(8)));
	unsigned char y[8] __attribute__((aligned(8)));
	unsigned char z[8] __attribute__((aligned(8)));
};

static_assert(sizeof(struct position) == sizeof(struct position_wire), "size of struct position not equal to wire protocol struct");
static_assert(sizeof(struct distance) == sizeof(struct distance_wire), "size of struct distance not equal to wire protocol struct");

void encode_position(struct position* input, struct position_wire* output) {
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

void encode_distance(struct distance* input, struct distance_wire* output) {
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

void decode_position(struct position_wire* input, struct position* output) {
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

void decode_distance(struct distance_wire* input, struct distance* output) {
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

int validate_position(const struct position* input) {
	return 0;
}

int validate_distance(const struct distance* input) {
	return 0;
}

void transform_position(struct position* input) {
}

void transform_distance(struct distance* input) {
}
