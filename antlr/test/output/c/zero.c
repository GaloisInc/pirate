#include <assert.h>
#include <endian.h>
#include <stdint.h>
#include <string.h>


__extension__ struct Zero {
};

__extension__ struct Zero_wire {
};

static_assert(sizeof(struct Zero) == sizeof(struct Zero_wire), "size of struct Zero not equal to wire protocol struct");

void encode_zero(struct Zero* input, struct Zero_wire* output) {
	(void) input;
	(void) output;
}

void decode_zero(struct Zero_wire* input, struct Zero* output) {
	(void) input;
	(void) output;
}
