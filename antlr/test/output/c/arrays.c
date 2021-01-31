#include <assert.h>
#include <endian.h>
#include <stdint.h>
#include <string.h>


struct Union_Array_Field {
	int16_t tag __attribute__((aligned(2)));
	union {
		uint8_t a __attribute__((aligned(1)));
		int32_t b[10] __attribute__((aligned(4)));
		float c[1][2][3] __attribute__((aligned(4)));
	} data;
};

struct Struct_Array_Field {
	uint8_t a __attribute__((aligned(1)));
	int32_t b[10] __attribute__((aligned(4)));
	float c[1][2][3][4][5][6] __attribute__((aligned(4)));
};

struct Union_Array_Field_wire {
	unsigned char tag[2];
	union {
		unsigned char a[1] __attribute__((aligned(1)));
		unsigned char b[10][4] __attribute__((aligned(4)));
		unsigned char c[1][2][3][4] __attribute__((aligned(4)));
	} data;
};

struct Struct_Array_Field_wire {
	unsigned char a[1] __attribute__((aligned(1)));
	unsigned char b[10][4] __attribute__((aligned(4)));
	unsigned char c[1][2][3][4][5][6][4] __attribute__((aligned(4)));
};

static_assert(sizeof(struct Union_Array_Field) == sizeof(struct Union_Array_Field_wire), "size of Union_Array_Field not equal to wire protocol size"
);
static_assert(sizeof(struct Struct_Array_Field) == sizeof(struct Struct_Array_Field_wire), "size of struct Struct_Array_Field not equal to wire protocol struct");

void encode_union_array_field(struct Union_Array_Field* input, struct Union_Array_Field_wire* output) {
	uint16_t tag;
	uint8_t data_a;
	uint32_t data_b;
	uint32_t data_c;
	memset(output, 0, sizeof(*output));
	memcpy(&tag, &input->tag, sizeof(uint16_t));
	tag = htobe16(tag);
	memcpy(&output->tag, &tag, sizeof(uint16_t));
	switch (input->tag) {
	case 1:
		memcpy(&data_a, &input->data.a, sizeof(uint8_t));
		memcpy(&output->data.a, &data_a, sizeof(uint8_t));
		break;
	case 2:
	case 3:
		for (size_t b_0 = 0; b_0 < 10; b_0++) {
			int32_t* inptr = &input->data.b[b_0];
			unsigned char* outptr = &output->data.b[b_0][0];
			memcpy(&data_b, inptr, sizeof(uint32_t));
			data_b = htobe32(data_b);
			memcpy(outptr, &data_b, sizeof(uint32_t));
		}
		break;
	case 4:
	default:
		for (size_t c_0 = 0; c_0 < 1; c_0++) {
			for (size_t c_1 = 0; c_1 < 2; c_1++) {
				for (size_t c_2 = 0; c_2 < 3; c_2++) {
					float* inptr = &input->data.c[c_0][c_1][c_2];
					unsigned char* outptr = &output->data.c[c_0][c_1][c_2][0];
					memcpy(&data_c, inptr, sizeof(uint32_t));
					data_c = htobe32(data_c);
					memcpy(outptr, &data_c, sizeof(uint32_t));
				}
			}
		}
		break;
	}
}

void encode_struct_array_field(struct Struct_Array_Field* input, struct Struct_Array_Field_wire* output) {
	uint8_t field_a;
	uint32_t field_b;
	uint32_t field_c;
	memset(output, 0, sizeof(*output));
	for (size_t b_0 = 0; b_0 < 10; b_0++) {
		int32_t* inptr = &input->b[b_0];
		unsigned char* outptr = &output->b[b_0][0];
		memcpy(&field_b, inptr, sizeof(uint32_t));
		field_b = htobe32(field_b);
		memcpy(outptr, &field_b, sizeof(uint32_t));
	}
	for (size_t c_0 = 0; c_0 < 1; c_0++) {
		for (size_t c_1 = 0; c_1 < 2; c_1++) {
			for (size_t c_2 = 0; c_2 < 3; c_2++) {
				for (size_t c_3 = 0; c_3 < 4; c_3++) {
					for (size_t c_4 = 0; c_4 < 5; c_4++) {
						for (size_t c_5 = 0; c_5 < 6; c_5++) {
							float* inptr = &input->c[c_0][c_1][c_2][c_3][c_4][c_5];
							unsigned char* outptr = &output->c[c_0][c_1][c_2][c_3][c_4][c_5][0];
							memcpy(&field_c, inptr, sizeof(uint32_t));
							field_c = htobe32(field_c);
							memcpy(outptr, &field_c, sizeof(uint32_t));
						}
					}
				}
			}
		}
	}
	memcpy(&field_a, &input->a, sizeof(uint8_t));
	memcpy(&output->a, &field_a, sizeof(uint8_t));
}

void decode_union_array_field(struct Union_Array_Field_wire* input, struct Union_Array_Field* output) {
	uint16_t tag;
	uint8_t data_a;
	uint32_t data_b;
	uint32_t data_c;
	memcpy(&tag, &input->tag, sizeof(uint16_t));
	tag = be16toh(tag);
	memcpy(&output->tag, &tag, sizeof(uint16_t));
	switch (output->tag) {
	case 1:
		memcpy(&data_a, &input->data.a, sizeof(uint8_t));
		memcpy(&output->data.a, &data_a, sizeof(uint8_t));
		break;
	case 2:
	case 3:
		for (size_t b_0 = 0; b_0 < 10; b_0++) {
			unsigned char* inptr = &input->data.b[b_0][0];
			int32_t* outptr = &output->data.b[b_0];
			memcpy(&data_b, inptr, sizeof(uint32_t));
			data_b = be32toh(data_b);
			memcpy(outptr, &data_b, sizeof(uint32_t));
		}
		break;
	case 4:
	default:
		for (size_t c_0 = 0; c_0 < 1; c_0++) {
			for (size_t c_1 = 0; c_1 < 2; c_1++) {
				for (size_t c_2 = 0; c_2 < 3; c_2++) {
					unsigned char* inptr = &input->data.c[c_0][c_1][c_2][0];
					float* outptr = &output->data.c[c_0][c_1][c_2];
					memcpy(&data_c, inptr, sizeof(uint32_t));
					data_c = be32toh(data_c);
					memcpy(outptr, &data_c, sizeof(uint32_t));
				}
			}
		}
		break;
	}
}

void decode_struct_array_field(struct Struct_Array_Field_wire* input, struct Struct_Array_Field* output) {
	uint8_t field_a;
	uint32_t field_b;
	uint32_t field_c;
	for (size_t b_0 = 0; b_0 < 10; b_0++) {
		unsigned char* inptr = &input->b[b_0][0];
		int32_t* outptr = &output->b[b_0];
		memcpy(&field_b, inptr, sizeof(uint32_t));
		field_b = be32toh(field_b);
		memcpy(outptr, &field_b, sizeof(uint32_t));
	}
	for (size_t c_0 = 0; c_0 < 1; c_0++) {
		for (size_t c_1 = 0; c_1 < 2; c_1++) {
			for (size_t c_2 = 0; c_2 < 3; c_2++) {
				for (size_t c_3 = 0; c_3 < 4; c_3++) {
					for (size_t c_4 = 0; c_4 < 5; c_4++) {
						for (size_t c_5 = 0; c_5 < 6; c_5++) {
							unsigned char* inptr = &input->c[c_0][c_1][c_2][c_3][c_4][c_5][0];
							float* outptr = &output->c[c_0][c_1][c_2][c_3][c_4][c_5];
							memcpy(&field_c, inptr, sizeof(uint32_t));
							field_c = be32toh(field_c);
							memcpy(outptr, &field_c, sizeof(uint32_t));
						}
					}
				}
			}
		}
	}
	memcpy(&field_a, &input->a, sizeof(uint8_t));
	memcpy(&output->a, &field_a, sizeof(uint8_t));
}
