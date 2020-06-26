#include <assert.h>
#include <endian.h>
#include <stdint.h>
#include <string.h>


struct union_array_field {
	int16_t tag __attribute__((aligned(2)));
	union {
		uint8_t a __attribute__((aligned(1)));
		int32_t b[10] __attribute__((aligned(4)));
		float c[1][2][3] __attribute__((aligned(4)));
	} data;
};

struct struct_array_field {
	uint8_t a __attribute__((aligned(1)));
	int32_t b[10] __attribute__((aligned(4)));
	float c[1][2][3][4][5][6] __attribute__((aligned(4)));
};

struct union_array_field_wire {
	unsigned char tag[2];
	union {
		unsigned char a[1] __attribute__((aligned(1)));
		unsigned char b[10][4] __attribute__((aligned(4)));
		unsigned char c[1][2][3][4] __attribute__((aligned(4)));
	} data;
};

struct struct_array_field_wire {
	unsigned char a[1] __attribute__((aligned(1)));
	unsigned char b[10][4] __attribute__((aligned(4)));
	unsigned char c[1][2][3][4][5][6][4] __attribute__((aligned(4)));
};

static_assert(sizeof(struct union_array_field) == sizeof(struct union_array_field_wire), "size of union_array_field not equal to wire protocol size"
);
static_assert(sizeof(struct struct_array_field) == sizeof(struct struct_array_field_wire), "size of struct struct_array_field not equal to wire protocol struct");

void encode_union_array_field(struct union_array_field* input, struct union_array_field_wire* output) {
	uint16_t tag;
	uint8_t data_a;
	uint32_t data_b;
	uint32_t data_c;
	memcpy(&tag, &input->tag, sizeof(uint16_t));
	tag = htobe16(tag);
	memcpy(&output->tag, &tag, sizeof(uint16_t));
	switch (tag) {
	case 1:
		memcpy(&data_a, &input->data.a, sizeof(uint8_t));
		memcpy(&output->data.a, &data_a, sizeof(uint8_t));
		break;
	case 2:
	case 3:
		for (size_t b_0 = 0; b_0 < 10; b_0++) {
			memcpy(&data_b, &input->data.b[b_0], sizeof(uint32_t));
			data_b = htobe32(data_b);
			memcpy(&output->data.b[b_0], &data_b, sizeof(uint32_t));
		}
		break;
	case 4:
	default:
		for (size_t c_0 = 0; c_0 < 1; c_0++) {
			for (size_t c_1 = 0; c_1 < 2; c_1++) {
				for (size_t c_2 = 0; c_2 < 3; c_2++) {
					memcpy(&data_c, &input->data.c[c_0][c_1][c_2], sizeof(uint32_t));
					data_c = htobe32(data_c);
					memcpy(&output->data.c[c_0][c_1][c_2], &data_c, sizeof(uint32_t));
				}
			}
		}
		break;
	}
}

void encode_struct_array_field(struct struct_array_field* input, struct struct_array_field_wire* output) {
	uint8_t a;
	uint32_t b;
	uint32_t c;
	for (size_t b_0 = 0; b_0 < 10; b_0++) {
		memcpy(&b, &input->b[b_0], sizeof(uint32_t));
		b = htobe32(b);
		memcpy(&output->b[b_0], &b, sizeof(uint32_t));
	}
	for (size_t c_0 = 0; c_0 < 1; c_0++) {
		for (size_t c_1 = 0; c_1 < 2; c_1++) {
			for (size_t c_2 = 0; c_2 < 3; c_2++) {
				for (size_t c_3 = 0; c_3 < 4; c_3++) {
					for (size_t c_4 = 0; c_4 < 5; c_4++) {
						for (size_t c_5 = 0; c_5 < 6; c_5++) {
							memcpy(&c, &input->c[c_0][c_1][c_2][c_3][c_4][c_5], sizeof(uint32_t));
							c = htobe32(c);
							memcpy(&output->c[c_0][c_1][c_2][c_3][c_4][c_5], &c, sizeof(uint32_t));
						}
					}
				}
			}
		}
	}
	memcpy(&a, &input->a, sizeof(uint8_t));
	memcpy(&output->a, &a, sizeof(uint8_t));
}

void decode_union_array_field(struct union_array_field_wire* input, struct union_array_field* output) {
	uint16_t tag;
	uint8_t data_a;
	uint32_t data_b;
	uint32_t data_c;
	memcpy(&tag, &input->tag, sizeof(uint16_t));
	tag = be16toh(tag);
	memcpy(&output->tag, &tag, sizeof(uint16_t));
	switch (tag) {
	case 1:
		memcpy(&data_a, &input->data.a, sizeof(uint8_t));
		memcpy(&output->data.a, &data_a, sizeof(uint8_t));
		break;
	case 2:
	case 3:
		for (size_t b_0 = 0; b_0 < 10; b_0++) {
			memcpy(&data_b, &input->data.b[b_0], sizeof(uint32_t));
			data_b = be32toh(data_b);
			memcpy(&output->data.b[b_0], &data_b, sizeof(uint32_t));
		}
		break;
	case 4:
	default:
		for (size_t c_0 = 0; c_0 < 1; c_0++) {
			for (size_t c_1 = 0; c_1 < 2; c_1++) {
				for (size_t c_2 = 0; c_2 < 3; c_2++) {
					memcpy(&data_c, &input->data.c[c_0][c_1][c_2], sizeof(uint32_t));
					data_c = be32toh(data_c);
					memcpy(&output->data.c[c_0][c_1][c_2], &data_c, sizeof(uint32_t));
				}
			}
		}
		break;
	}
}

void decode_struct_array_field(struct struct_array_field_wire* input, struct struct_array_field* output) {
	uint8_t a;
	uint32_t b;
	uint32_t c;
	for (size_t b_0 = 0; b_0 < 10; b_0++) {
		memcpy(&b, &input->b[b_0], sizeof(uint32_t));
		b = be32toh(b);
		memcpy(&output->b[b_0], &b, sizeof(uint32_t));
	}
	for (size_t c_0 = 0; c_0 < 1; c_0++) {
		for (size_t c_1 = 0; c_1 < 2; c_1++) {
			for (size_t c_2 = 0; c_2 < 3; c_2++) {
				for (size_t c_3 = 0; c_3 < 4; c_3++) {
					for (size_t c_4 = 0; c_4 < 5; c_4++) {
						for (size_t c_5 = 0; c_5 < 6; c_5++) {
							memcpy(&c, &input->c[c_0][c_1][c_2][c_3][c_4][c_5], sizeof(uint32_t));
							c = be32toh(c);
							memcpy(&output->c[c_0][c_1][c_2][c_3][c_4][c_5], &c, sizeof(uint32_t));
						}
					}
				}
			}
		}
	}
	memcpy(&a, &input->a, sizeof(uint8_t));
	memcpy(&output->a, &a, sizeof(uint8_t));
}
