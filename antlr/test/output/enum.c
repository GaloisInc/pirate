#include <endian.h>
#include <stdint.h>
#include <string.h>


enum dayofweek {
Monday,
Tuesday,
Wednesday,
Thursday,
Friday
};

struct week_interval {
	uint32_t begin __attribute__((aligned(4)));
	uint32_t end __attribute__((aligned(4)));
};

uint32_t encode_dayofweek(uint32_t value) {
	value = htobe32(value);
	return value;
}

void encode_week_interval(struct week_interval* input, struct week_interval* output) {
	uint32_t begin;
	uint32_t end;
	memcpy(&begin, &input->begin, sizeof(uint32_t));
	memcpy(&end, &input->end, sizeof(uint32_t));
	begin = htobe32(begin);
	end = htobe32(end);
	memcpy(&output->begin, &begin, sizeof(uint32_t));
	memcpy(&output->end, &end, sizeof(uint32_t));
}

uint32_t decode_dayofweek(uint32_t value) {
	value = be32toh(value);
	return value;
}

void decode_week_interval(struct week_interval* input, struct week_interval* output) {
	uint32_t begin;
	uint32_t end;
	memcpy(&begin, &input->begin, sizeof(uint32_t));
	memcpy(&end, &input->end, sizeof(uint32_t));
	begin = be32toh(begin);
	end = be32toh(end);
	memcpy(&output->begin, &begin, sizeof(uint32_t));
	memcpy(&output->end, &end, sizeof(uint32_t));
}
