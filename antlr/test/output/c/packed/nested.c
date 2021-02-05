#include <assert.h>
#include <endian.h>
#include <stdint.h>
#include <string.h>


struct Foo {
	int32_t a __attribute__((aligned(4)));
	int32_t b __attribute__((aligned(4)));
	int32_t c __attribute__((aligned(4)));
};

struct Bar {
	double x __attribute__((aligned(8)));
	double y __attribute__((aligned(8)));
	double z __attribute__((aligned(8)));
};

enum DayOfWeek {
	Monday,
	Tuesday,
	Wednesday,
	Thursday,
	Friday
};

struct OuterStruct {
	struct Foo foo;
	struct Bar bar[2][3][4];
	uint32_t day __attribute__((aligned(4)));
	uint32_t days[30] __attribute__((aligned(4)));
};

struct OuterUnion {
	uint32_t tag __attribute__((aligned(4)));
	union {
		uint32_t day __attribute__((aligned(4)));
		uint32_t days[30] __attribute__((aligned(4)));
		struct Foo foo;
		struct Bar bar[2][3][4];
	} data;
};

struct ScopedOuterUnion {
	uint32_t tag __attribute__((aligned(4)));
	union {
		uint32_t day __attribute__((aligned(4)));
		uint32_t days[30] __attribute__((aligned(4)));
		struct Foo foo;
		struct Bar bar[2][3][4];
	} data;
};

struct Foo_wire {
	unsigned char a[4];
	unsigned char b[4];
	unsigned char c[4];
} __attribute__((packed)) ;

struct Bar_wire {
	unsigned char x[8];
	unsigned char y[8];
	unsigned char z[8];
} __attribute__((packed)) ;

struct OuterStruct_wire {
	struct Foo_wire foo;
	struct Bar_wire bar[2][3][4];
	unsigned char day[4];
	unsigned char days[30][4];
} __attribute__((packed)) ;

struct OuterUnion_wire {
	unsigned char tag[4];
	union {
		unsigned char day[4];
		unsigned char days[30][4];
		struct Foo_wire foo;
		struct Bar_wire bar[2][3][4];
	} data;
} __attribute__((packed)) ;

struct ScopedOuterUnion_wire {
	unsigned char tag[4];
	union {
		unsigned char day[4];
		unsigned char days[30][4];
		struct Foo_wire foo;
		struct Bar_wire bar[2][3][4];
	} data;
} __attribute__((packed)) ;


void encode_foo(struct Foo* input, struct Foo_wire* output) {
	uint32_t field_a;
	uint32_t field_b;
	uint32_t field_c;
	memcpy(&field_a, &input->a, sizeof(uint32_t));
	memcpy(&field_b, &input->b, sizeof(uint32_t));
	memcpy(&field_c, &input->c, sizeof(uint32_t));
	field_a = htobe32(field_a);
	field_b = htobe32(field_b);
	field_c = htobe32(field_c);
	memcpy(&output->a, &field_a, sizeof(uint32_t));
	memcpy(&output->b, &field_b, sizeof(uint32_t));
	memcpy(&output->c, &field_c, sizeof(uint32_t));
}

void encode_bar(struct Bar* input, struct Bar_wire* output) {
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

uint32_t encode_dayofweek(uint32_t value) {
	value = htobe32(value);
	return value;
}

void encode_outerstruct(struct OuterStruct* input, struct OuterStruct_wire* output) {
	uint32_t field_day;
	uint32_t field_days;
	for (size_t days_0 = 0; days_0 < 30; days_0++) {
		uint32_t* inptr = &input->days[days_0];
		unsigned char* outptr = &output->days[days_0][0];
		memcpy(&field_days, inptr, sizeof(uint32_t));
		field_days = htobe32(field_days);
		memcpy(outptr, &field_days, sizeof(uint32_t));
	}
	memcpy(&field_day, &input->day, sizeof(uint32_t));
	field_day = htobe32(field_day);
	memcpy(&output->day, &field_day, sizeof(uint32_t));
	encode_foo(&input->foo, &output->foo);
	for (size_t bar_0 = 0; bar_0 < 2; bar_0++) {
		for (size_t bar_1 = 0; bar_1 < 3; bar_1++) {
			for (size_t bar_2 = 0; bar_2 < 4; bar_2++) {
				struct Bar* inptr = &input->bar[bar_0][bar_1][bar_2];
				struct Bar_wire* outptr = &output->bar[bar_0][bar_1][bar_2];
				encode_bar(inptr, outptr);
			}
		}
	}
}

void encode_outerunion(struct OuterUnion* input, struct OuterUnion_wire* output) {
	uint32_t tag;
	uint32_t data_day;
	uint32_t data_days;
	memset(output, 0, sizeof(*output));
	memcpy(&tag, &input->tag, sizeof(uint32_t));
	tag = htobe32(tag);
	memcpy(&output->tag, &tag, sizeof(uint32_t));
	switch (input->tag) {
	case Monday:
		memcpy(&data_day, &input->data.day, sizeof(uint32_t));
		data_day = htobe32(data_day);
		memcpy(&output->data.day, &data_day, sizeof(uint32_t));
		break;
	case Tuesday:
		for (size_t days_0 = 0; days_0 < 30; days_0++) {
			uint32_t* inptr = &input->data.days[days_0];
			unsigned char* outptr = &output->data.days[days_0][0];
			memcpy(&data_days, inptr, sizeof(uint32_t));
			data_days = htobe32(data_days);
			memcpy(outptr, &data_days, sizeof(uint32_t));
		}
		break;
	case Wednesday:
		encode_foo(&input->data.foo, &output->data.foo);
		break;
	case Thursday:
	case Friday:
		for (size_t bar_0 = 0; bar_0 < 2; bar_0++) {
			for (size_t bar_1 = 0; bar_1 < 3; bar_1++) {
				for (size_t bar_2 = 0; bar_2 < 4; bar_2++) {
					struct Bar* inptr = &input->data.bar[bar_0][bar_1][bar_2];
					struct Bar_wire* outptr = &output->data.bar[bar_0][bar_1][bar_2];
					encode_bar(inptr, outptr);
				}
			}
		}
		break;
	}
}

void encode_scopedouterunion(struct ScopedOuterUnion* input, struct ScopedOuterUnion_wire* output) {
	uint32_t tag;
	uint32_t data_day;
	uint32_t data_days;
	memset(output, 0, sizeof(*output));
	memcpy(&tag, &input->tag, sizeof(uint32_t));
	tag = htobe32(tag);
	memcpy(&output->tag, &tag, sizeof(uint32_t));
	switch (input->tag) {
	case Monday:
		memcpy(&data_day, &input->data.day, sizeof(uint32_t));
		data_day = htobe32(data_day);
		memcpy(&output->data.day, &data_day, sizeof(uint32_t));
		break;
	case Tuesday:
		for (size_t days_0 = 0; days_0 < 30; days_0++) {
			uint32_t* inptr = &input->data.days[days_0];
			unsigned char* outptr = &output->data.days[days_0][0];
			memcpy(&data_days, inptr, sizeof(uint32_t));
			data_days = htobe32(data_days);
			memcpy(outptr, &data_days, sizeof(uint32_t));
		}
		break;
	case Wednesday:
		encode_foo(&input->data.foo, &output->data.foo);
		break;
	case Thursday:
	case Friday:
		for (size_t bar_0 = 0; bar_0 < 2; bar_0++) {
			for (size_t bar_1 = 0; bar_1 < 3; bar_1++) {
				for (size_t bar_2 = 0; bar_2 < 4; bar_2++) {
					struct Bar* inptr = &input->data.bar[bar_0][bar_1][bar_2];
					struct Bar_wire* outptr = &output->data.bar[bar_0][bar_1][bar_2];
					encode_bar(inptr, outptr);
				}
			}
		}
		break;
	}
}

void decode_foo(struct Foo_wire* input, struct Foo* output) {
	uint32_t field_a;
	uint32_t field_b;
	uint32_t field_c;
	memcpy(&field_a, &input->a, sizeof(uint32_t));
	memcpy(&field_b, &input->b, sizeof(uint32_t));
	memcpy(&field_c, &input->c, sizeof(uint32_t));
	field_a = be32toh(field_a);
	field_b = be32toh(field_b);
	field_c = be32toh(field_c);
	memcpy(&output->a, &field_a, sizeof(uint32_t));
	memcpy(&output->b, &field_b, sizeof(uint32_t));
	memcpy(&output->c, &field_c, sizeof(uint32_t));
}

void decode_bar(struct Bar_wire* input, struct Bar* output) {
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

uint32_t decode_dayofweek(uint32_t value) {
	value = be32toh(value);
	return value;
}

void decode_outerstruct(struct OuterStruct_wire* input, struct OuterStruct* output) {
	uint32_t field_day;
	uint32_t field_days;
	for (size_t days_0 = 0; days_0 < 30; days_0++) {
		unsigned char* inptr = &input->days[days_0][0];
		uint32_t* outptr = &output->days[days_0];
		memcpy(&field_days, inptr, sizeof(uint32_t));
		field_days = be32toh(field_days);
		memcpy(outptr, &field_days, sizeof(uint32_t));
	}
	memcpy(&field_day, &input->day, sizeof(uint32_t));
	field_day = be32toh(field_day);
	memcpy(&output->day, &field_day, sizeof(uint32_t));
	decode_foo(&input->foo, &output->foo);
	for (size_t bar_0 = 0; bar_0 < 2; bar_0++) {
		for (size_t bar_1 = 0; bar_1 < 3; bar_1++) {
			for (size_t bar_2 = 0; bar_2 < 4; bar_2++) {
				struct Bar_wire* inptr = &input->bar[bar_0][bar_1][bar_2];
				struct Bar* outptr = &output->bar[bar_0][bar_1][bar_2];
				decode_bar(inptr, outptr);
			}
		}
	}
}

void decode_outerunion(struct OuterUnion_wire* input, struct OuterUnion* output) {
	uint32_t tag;
	uint32_t data_day;
	uint32_t data_days;
	memcpy(&tag, &input->tag, sizeof(uint32_t));
	tag = be32toh(tag);
	memcpy(&output->tag, &tag, sizeof(uint32_t));
	switch (output->tag) {
	case Monday:
		memcpy(&data_day, &input->data.day, sizeof(uint32_t));
		data_day = be32toh(data_day);
		memcpy(&output->data.day, &data_day, sizeof(uint32_t));
		break;
	case Tuesday:
		for (size_t days_0 = 0; days_0 < 30; days_0++) {
			unsigned char* inptr = &input->data.days[days_0][0];
			uint32_t* outptr = &output->data.days[days_0];
			memcpy(&data_days, inptr, sizeof(uint32_t));
			data_days = be32toh(data_days);
			memcpy(outptr, &data_days, sizeof(uint32_t));
		}
		break;
	case Wednesday:
		decode_foo(&input->data.foo, &output->data.foo);
		break;
	case Thursday:
	case Friday:
		for (size_t bar_0 = 0; bar_0 < 2; bar_0++) {
			for (size_t bar_1 = 0; bar_1 < 3; bar_1++) {
				for (size_t bar_2 = 0; bar_2 < 4; bar_2++) {
					struct Bar_wire* inptr = &input->data.bar[bar_0][bar_1][bar_2];
					struct Bar* outptr = &output->data.bar[bar_0][bar_1][bar_2];
					decode_bar(inptr, outptr);
				}
			}
		}
		break;
	}
}

void decode_scopedouterunion(struct ScopedOuterUnion_wire* input, struct ScopedOuterUnion* output) {
	uint32_t tag;
	uint32_t data_day;
	uint32_t data_days;
	memcpy(&tag, &input->tag, sizeof(uint32_t));
	tag = be32toh(tag);
	memcpy(&output->tag, &tag, sizeof(uint32_t));
	switch (output->tag) {
	case Monday:
		memcpy(&data_day, &input->data.day, sizeof(uint32_t));
		data_day = be32toh(data_day);
		memcpy(&output->data.day, &data_day, sizeof(uint32_t));
		break;
	case Tuesday:
		for (size_t days_0 = 0; days_0 < 30; days_0++) {
			unsigned char* inptr = &input->data.days[days_0][0];
			uint32_t* outptr = &output->data.days[days_0];
			memcpy(&data_days, inptr, sizeof(uint32_t));
			data_days = be32toh(data_days);
			memcpy(outptr, &data_days, sizeof(uint32_t));
		}
		break;
	case Wednesday:
		decode_foo(&input->data.foo, &output->data.foo);
		break;
	case Thursday:
	case Friday:
		for (size_t bar_0 = 0; bar_0 < 2; bar_0++) {
			for (size_t bar_1 = 0; bar_1 < 3; bar_1++) {
				for (size_t bar_2 = 0; bar_2 < 4; bar_2++) {
					struct Bar_wire* inptr = &input->data.bar[bar_0][bar_1][bar_2];
					struct Bar* outptr = &output->data.bar[bar_0][bar_1][bar_2];
					decode_bar(inptr, outptr);
				}
			}
		}
		break;
	}
}
