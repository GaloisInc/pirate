#include <assert.h>
#include <endian.h>
#include <fenv.h>
#include <math.h>
#include <stdint.h>
#include <string.h>


struct union_example {
	int16_t tag __attribute__((aligned(2)));
	union {
		uint8_t a __attribute__((aligned(1)));
		int32_t b __attribute__((aligned(4)));
		float c __attribute__((aligned(4)));
	} data;
};

struct union_example_wire {
	unsigned char tag[2];
	union {
		unsigned char a[1] __attribute__((aligned(1)));
		unsigned char b[4] __attribute__((aligned(4)));
		unsigned char c[4] __attribute__((aligned(4)));
	} data;
};

static_assert(sizeof(struct union_example) == sizeof(struct union_example_wire), "size of union_example not equal to wire protocol size"
);

void encode_union_example(struct union_example* input, struct union_example_wire* output) {
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

void decode_union_example(struct union_example_wire* input, struct union_example* output) {
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

int validate_union_example(const struct union_example* input) {
	switch (input->tag) {
	case 1:
		break;
	case 2:
	case 3:
		break;
	case 4:
	default:
		break;
	}
	return 0;
}

void transform_union_example(struct union_example* input) {
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
			break;
		}
	}
}
