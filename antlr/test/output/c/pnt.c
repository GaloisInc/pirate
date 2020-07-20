#include <assert.h>
#include <endian.h>
#include <stdint.h>
#include <string.h>


struct Position {
	double x __attribute__((aligned(8)));
	double y __attribute__((aligned(8)));
	double z __attribute__((aligned(8)));
};

struct Distance {
	double x __attribute__((aligned(8)));
	double y __attribute__((aligned(8)));
	double z __attribute__((aligned(8)));
};

struct Position_wire {
	unsigned char x[8] __attribute__((aligned(8)));
	unsigned char y[8] __attribute__((aligned(8)));
	unsigned char z[8] __attribute__((aligned(8)));
};

struct Distance_wire {
	unsigned char x[8] __attribute__((aligned(8)));
	unsigned char y[8] __attribute__((aligned(8)));
	unsigned char z[8] __attribute__((aligned(8)));
};

static_assert(sizeof(struct Position) == sizeof(struct Position_wire), "size of struct Position not equal to wire protocol struct");
static_assert(sizeof(struct Distance) == sizeof(struct Distance_wire), "size of struct Distance not equal to wire protocol struct");

void encode_position(struct Position* input, struct Position_wire* output) {
	uint64_t field_x;
	uint64_t field_y;
	uint64_t field_z;
	memcpy(&field_x, &input->x, sizeof(uint64_t));
	memcpy(&field_y, &input->y, sizeof(uint64_t));
	memcpy(&field_z, &input->z, sizeof(uint64_t));
	field_x = htobe64(field_x);
	field_y = htobe64(field_y);
	field_z = htobe64(field_z);
	memcpy(&output->x, &field_x, sizeof(uint64_t));
	memcpy(&output->y, &field_y, sizeof(uint64_t));
	memcpy(&output->z, &field_z, sizeof(uint64_t));
}

void encode_distance(struct Distance* input, struct Distance_wire* output) {
	uint64_t field_x;
	uint64_t field_y;
	uint64_t field_z;
	memcpy(&field_x, &input->x, sizeof(uint64_t));
	memcpy(&field_y, &input->y, sizeof(uint64_t));
	memcpy(&field_z, &input->z, sizeof(uint64_t));
	field_x = htobe64(field_x);
	field_y = htobe64(field_y);
	field_z = htobe64(field_z);
	memcpy(&output->x, &field_x, sizeof(uint64_t));
	memcpy(&output->y, &field_y, sizeof(uint64_t));
	memcpy(&output->z, &field_z, sizeof(uint64_t));
}

void decode_position(struct Position_wire* input, struct Position* output) {
	uint64_t field_x;
	uint64_t field_y;
	uint64_t field_z;
	memcpy(&field_x, &input->x, sizeof(uint64_t));
	memcpy(&field_y, &input->y, sizeof(uint64_t));
	memcpy(&field_z, &input->z, sizeof(uint64_t));
	field_x = be64toh(field_x);
	field_y = be64toh(field_y);
	field_z = be64toh(field_z);
	memcpy(&output->x, &field_x, sizeof(uint64_t));
	memcpy(&output->y, &field_y, sizeof(uint64_t));
	memcpy(&output->z, &field_z, sizeof(uint64_t));
}

void decode_distance(struct Distance_wire* input, struct Distance* output) {
	uint64_t field_x;
	uint64_t field_y;
	uint64_t field_z;
	memcpy(&field_x, &input->x, sizeof(uint64_t));
	memcpy(&field_y, &input->y, sizeof(uint64_t));
	memcpy(&field_z, &input->z, sizeof(uint64_t));
	field_x = be64toh(field_x);
	field_y = be64toh(field_y);
	field_z = be64toh(field_z);
	memcpy(&output->x, &field_x, sizeof(uint64_t));
	memcpy(&output->y, &field_y, sizeof(uint64_t));
	memcpy(&output->z, &field_z, sizeof(uint64_t));
}
