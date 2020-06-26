#include <assert.h>
#include <endian.h>
#include <fenv.h>
#include <math.h>
#include <stdint.h>
#include <string.h>


struct annotation_struct_example {
	double x __attribute__((aligned(8)));
	double y __attribute__((aligned(8)));
	double z __attribute__((aligned(8)));
};

struct annotation_union_example {
	int16_t tag __attribute__((aligned(2)));
	union {
		int16_t a __attribute__((aligned(2)));
		int32_t b __attribute__((aligned(4)));
		float c __attribute__((aligned(4)));
	} data;
};

struct annotation_struct_example_wire {
	unsigned char x[8] __attribute__((aligned(8)));
	unsigned char y[8] __attribute__((aligned(8)));
	unsigned char z[8] __attribute__((aligned(8)));
};

struct annotation_union_example_wire {
	unsigned char tag[2];
	union {
		unsigned char a[2] __attribute__((aligned(2)));
		unsigned char b[4] __attribute__((aligned(4)));
		unsigned char c[4] __attribute__((aligned(4)));
	} data;
};

static_assert(sizeof(struct annotation_struct_example) == sizeof(struct annotation_struct_example_wire), "size of struct annotation_struct_example not equal to wire protocol struct");
static_assert(sizeof(struct annotation_union_example) == sizeof(struct annotation_union_example_wire), "size of annotation_union_example not equal to wire protocol size"
);

void encode_annotation_struct_example(struct annotation_struct_example* input, struct annotation_struct_example_wire* output) {
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

void encode_annotation_union_example(struct annotation_union_example* input, struct annotation_union_example_wire* output) {
	uint16_t tag;
	uint16_t data_a;
	uint32_t data_b;
	uint32_t data_c;
	memcpy(&tag, &input->tag, sizeof(uint16_t));
	tag = htobe16(tag);
	memcpy(&output->tag, &tag, sizeof(uint16_t));
	switch (tag) {
	case 1:
		memcpy(&data_a, &input->data.a, sizeof(uint16_t));
		data_a = htobe16(data_a);
		memcpy(&output->data.a, &data_a, sizeof(uint16_t));
		break;
	case 2:
	case 3:
		memcpy(&data_b, &input->data.b, sizeof(uint32_t));
		data_b = htobe32(data_b);
		memcpy(&output->data.b, &data_b, sizeof(uint32_t));
		break;
	case 4:
	default:
		memcpy(&data_c, &input->data.c, sizeof(uint32_t));
		data_c = htobe32(data_c);
		memcpy(&output->data.c, &data_c, sizeof(uint32_t));
		break;
	}
}

void decode_annotation_struct_example(struct annotation_struct_example_wire* input, struct annotation_struct_example* output) {
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

void decode_annotation_union_example(struct annotation_union_example_wire* input, struct annotation_union_example* output) {
	uint16_t tag;
	uint16_t data_a;
	uint32_t data_b;
	uint32_t data_c;
	memcpy(&tag, &input->tag, sizeof(uint16_t));
	tag = be16toh(tag);
	memcpy(&output->tag, &tag, sizeof(uint16_t));
	switch (tag) {
	case 1:
		memcpy(&data_a, &input->data.a, sizeof(uint16_t));
		data_a = be16toh(data_a);
		memcpy(&output->data.a, &data_a, sizeof(uint16_t));
		break;
	case 2:
	case 3:
		memcpy(&data_b, &input->data.b, sizeof(uint32_t));
		data_b = be32toh(data_b);
		memcpy(&output->data.b, &data_b, sizeof(uint32_t));
		break;
	case 4:
	default:
		memcpy(&data_c, &input->data.c, sizeof(uint32_t));
		data_c = be32toh(data_c);
		memcpy(&output->data.c, &data_c, sizeof(uint32_t));
		break;
	}
}

int validate_annotation_struct_example(const struct annotation_struct_example* input) {
	if (input->x < 0) {
		return -1;
	}
	if (input->y > 10) {
		return -1;
	}
	if ((input->z < 0) || (input->z > 10)) {
		return -1;
	}
	return 0;
}

int validate_annotation_union_example(const struct annotation_union_example* input) {
	switch (input->tag) {
	case 1:
		if (input->data.a < 0) {
			return -1;
		}
		break;
	case 2:
	case 3:
		if (input->data.b > 10) {
			return -1;
		}
		break;
	case 4:
	default:
		if ((input->data.c < 0) || (input->data.c > 10)) {
			return -1;
		}
		break;
	}
	return 0;
}

void transform_annotation_struct_example(struct annotation_struct_example* input) {
	int rmode = fegetround();
	fesetround(FE_TONEAREST);
	input->z = nearbyint(input->z);
	fesetround(rmode);
}

void transform_annotation_union_example(struct annotation_union_example* input) {
	switch (input->tag) {
		case 1:
		{
			break;
		}
		case 2:
		case 3:
		{
			break;
		}
		case 4:
		default:
		{
			int rmode = fegetround();
			fesetround(FE_TONEAREST);
			input->data.c = nearbyintf(input->data.c);
			fesetround(rmode);
			break;
		}
	}
}
