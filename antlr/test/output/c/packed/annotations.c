#include <assert.h>
#include <endian.h>
#include <fenv.h>
#include <math.h>
#include <stdint.h>
#include <string.h>


struct Annotation_Struct_Example {
	int32_t u __attribute__((aligned(4)));
	float v __attribute__((aligned(4)));
	double x __attribute__((aligned(8)));
	double y __attribute__((aligned(8)));
	double z __attribute__((aligned(8)));
};

struct Annotation_Union_Example {
	int16_t tag __attribute__((aligned(2)));
	union {
		int16_t a __attribute__((aligned(2)));
		int32_t b __attribute__((aligned(4)));
		float c __attribute__((aligned(4)));
	} data;
};

struct Annotation_Struct_Example_wire {
	unsigned char u[4];
	unsigned char v[4];
	unsigned char x[8];
	unsigned char y[8];
	unsigned char z[8];
} __attribute__((packed)) ;

struct Annotation_Union_Example_wire {
	unsigned char tag[2];
	union {
		unsigned char a[2];
		unsigned char b[4];
		unsigned char c[4];
	} data;
} __attribute__((packed)) ;


void encode_annotation_struct_example(struct Annotation_Struct_Example* input, struct Annotation_Struct_Example_wire* output) {
	uint32_t field_u;
	uint32_t field_v;
	uint64_t field_x;
	uint64_t field_y;
	uint64_t field_z;
	memcpy(&field_u, &input->u, sizeof(uint32_t));
	memcpy(&field_v, &input->v, sizeof(uint32_t));
	memcpy(&field_x, &input->x, sizeof(uint64_t));
	memcpy(&field_y, &input->y, sizeof(uint64_t));
	memcpy(&field_z, &input->z, sizeof(uint64_t));
	field_u = htobe32(field_u);
	field_v = htobe32(field_v);
	field_x = htobe64(field_x);
	field_y = htobe64(field_y);
	field_z = htobe64(field_z);
	memcpy(&output->u, &field_u, sizeof(uint32_t));
	memcpy(&output->v, &field_v, sizeof(uint32_t));
	memcpy(&output->x, &field_x, sizeof(uint64_t));
	memcpy(&output->y, &field_y, sizeof(uint64_t));
	memcpy(&output->z, &field_z, sizeof(uint64_t));
}

void encode_annotation_union_example(struct Annotation_Union_Example* input, struct Annotation_Union_Example_wire* output) {
	uint16_t tag;
	uint16_t data_a;
	uint32_t data_b;
	uint32_t data_c;
	memset(output, 0, sizeof(*output));
	memcpy(&tag, &input->tag, sizeof(uint16_t));
	tag = htobe16(tag);
	memcpy(&output->tag, &tag, sizeof(uint16_t));
	switch (input->tag) {
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

void decode_annotation_struct_example(struct Annotation_Struct_Example_wire* input, struct Annotation_Struct_Example* output) {
	uint32_t field_u;
	uint32_t field_v;
	uint64_t field_x;
	uint64_t field_y;
	uint64_t field_z;
	memcpy(&field_u, &input->u, sizeof(uint32_t));
	memcpy(&field_v, &input->v, sizeof(uint32_t));
	memcpy(&field_x, &input->x, sizeof(uint64_t));
	memcpy(&field_y, &input->y, sizeof(uint64_t));
	memcpy(&field_z, &input->z, sizeof(uint64_t));
	field_u = be32toh(field_u);
	field_v = be32toh(field_v);
	field_x = be64toh(field_x);
	field_y = be64toh(field_y);
	field_z = be64toh(field_z);
	memcpy(&output->u, &field_u, sizeof(uint32_t));
	memcpy(&output->v, &field_v, sizeof(uint32_t));
	memcpy(&output->x, &field_x, sizeof(uint64_t));
	memcpy(&output->y, &field_y, sizeof(uint64_t));
	memcpy(&output->z, &field_z, sizeof(uint64_t));
}

void decode_annotation_union_example(struct Annotation_Union_Example_wire* input, struct Annotation_Union_Example* output) {
	uint16_t tag;
	uint16_t data_a;
	uint32_t data_b;
	uint32_t data_c;
	memcpy(&tag, &input->tag, sizeof(uint16_t));
	tag = be16toh(tag);
	memcpy(&output->tag, &tag, sizeof(uint16_t));
	switch (output->tag) {
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

int validate_annotation_struct_example(const struct Annotation_Struct_Example* input) {
	if (input->u != 0) {
		return -1;
	}
	if (input->v != 10) {
		return -1;
	}
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

int validate_annotation_union_example(const struct Annotation_Union_Example* input) {
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

void transform_annotation_struct_example(struct Annotation_Struct_Example* input) {
	int rmode = fegetround();
	fesetround(FE_TONEAREST);
	input->z = nearbyint(input->z);
	fesetround(rmode);
}

void transform_annotation_union_example(struct Annotation_Union_Example* input) {
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
