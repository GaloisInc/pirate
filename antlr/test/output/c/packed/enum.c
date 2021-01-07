#include <assert.h>
#include <endian.h>
#include <stdint.h>
#include <string.h>


enum DayOfWeek {
	Monday,
	Tuesday,
	Wednesday,
	Thursday,
	Friday
};

struct Week_Interval {
	uint32_t begin __attribute__((aligned(4)));
	uint32_t end __attribute__((aligned(4)));
};

struct Week_Interval_wire {
	unsigned char begin[4];
	unsigned char end[4];
} __attribute__((packed)) ;


uint32_t encode_dayofweek(uint32_t value) {
	value = htobe32(value);
	return value;
}

void encode_week_interval(struct Week_Interval* input, struct Week_Interval_wire* output) {
	uint32_t field_begin;
	uint32_t field_end;
	memcpy(&field_begin, &input->begin, sizeof(uint32_t));
	memcpy(&field_end, &input->end, sizeof(uint32_t));
	field_begin = htobe32(field_begin);
	field_end = htobe32(field_end);
	memcpy(&output->begin, &field_begin, sizeof(uint32_t));
	memcpy(&output->end, &field_end, sizeof(uint32_t));
}

uint32_t decode_dayofweek(uint32_t value) {
	value = be32toh(value);
	return value;
}

void decode_week_interval(struct Week_Interval_wire* input, struct Week_Interval* output) {
	uint32_t field_begin;
	uint32_t field_end;
	memcpy(&field_begin, &input->begin, sizeof(uint32_t));
	memcpy(&field_end, &input->end, sizeof(uint32_t));
	field_begin = be32toh(field_begin);
	field_end = be32toh(field_end);
	memcpy(&output->begin, &field_begin, sizeof(uint32_t));
	memcpy(&output->end, &field_end, sizeof(uint32_t));
}
