#include <cassert>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

#include <endian.h>

namespace primitives {

	struct primitives {
		float float_val __attribute__((aligned(4)));
		double double_val __attribute__((aligned(8)));
		int16_t short_val __attribute__((aligned(2)));
		int16_t int16_val __attribute__((aligned(2)));
		int32_t long_val __attribute__((aligned(4)));
		int32_t int32_val __attribute__((aligned(4)));
		int64_t long_long_val __attribute__((aligned(8)));
		int64_t int64_val __attribute__((aligned(8)));
		uint16_t unsigned_short_val __attribute__((aligned(2)));
		uint16_t uint16_val __attribute__((aligned(2)));
		uint32_t unsigned_long_val __attribute__((aligned(4)));
		uint32_t uint32_val __attribute__((aligned(4)));
		uint64_t unsigned_long_long_val __attribute__((aligned(8)));
		uint64_t uint64_val __attribute__((aligned(8)));
		char char_val __attribute__((aligned(1)));
		int8_t int8_val __attribute__((aligned(1)));
		uint8_t bool_val __attribute__((aligned(1)));
		uint8_t octet_val __attribute__((aligned(1)));
		uint8_t uint8_val __attribute__((aligned(1)));
	};

	struct primitives_wire {
		unsigned char float_val[4] __attribute__((aligned(4)));
		unsigned char double_val[8] __attribute__((aligned(8)));
		unsigned char short_val[2] __attribute__((aligned(2)));
		unsigned char int16_val[2] __attribute__((aligned(2)));
		unsigned char long_val[4] __attribute__((aligned(4)));
		unsigned char int32_val[4] __attribute__((aligned(4)));
		unsigned char long_long_val[8] __attribute__((aligned(8)));
		unsigned char int64_val[8] __attribute__((aligned(8)));
		unsigned char unsigned_short_val[2] __attribute__((aligned(2)));
		unsigned char uint16_val[2] __attribute__((aligned(2)));
		unsigned char unsigned_long_val[4] __attribute__((aligned(4)));
		unsigned char uint32_val[4] __attribute__((aligned(4)));
		unsigned char unsigned_long_long_val[8] __attribute__((aligned(8)));
		unsigned char uint64_val[8] __attribute__((aligned(8)));
		unsigned char char_val[1] __attribute__((aligned(1)));
		unsigned char int8_val[1] __attribute__((aligned(1)));
		unsigned char bool_val[1] __attribute__((aligned(1)));
		unsigned char octet_val[1] __attribute__((aligned(1)));
		unsigned char uint8_val[1] __attribute__((aligned(1)));
	};

	static_assert(sizeof(struct primitives) == sizeof(struct primitives_wire), "size of struct primitives not equal to wire protocol struct");
}

namespace pirate {
#ifndef _PIRATE_SERIALIZATION_H
#define _PIRATE_SERIALIZATION_H
	template <typename T>
	struct Serialization {
		static void toBuffer(T const& val, std::vector<char>& buf);
		static T fromBuffer(std::vector<char> const& buf);
	};
#endif // _PIRATE_SERIALIZATION_H

	template<>
	struct Serialization<struct primitives::primitives> {
		static void toBuffer(struct primitives::primitives const& val, std::vector<char>& buf) {
			buf.resize(sizeof(struct primitives::primitives));
			struct primitives::primitives_wire* output = (struct primitives::primitives_wire*) buf.data();
			const struct primitives::primitives* input = &val;
			uint32_t field_float_val;
			uint64_t field_double_val;
			uint16_t field_short_val;
			uint16_t field_int16_val;
			uint32_t field_long_val;
			uint32_t field_int32_val;
			uint64_t field_long_long_val;
			uint64_t field_int64_val;
			uint16_t field_unsigned_short_val;
			uint16_t field_uint16_val;
			uint32_t field_unsigned_long_val;
			uint32_t field_uint32_val;
			uint64_t field_unsigned_long_long_val;
			uint64_t field_uint64_val;
			uint8_t field_char_val;
			uint8_t field_int8_val;
			uint8_t field_bool_val;
			uint8_t field_octet_val;
			uint8_t field_uint8_val;
			memcpy(&field_float_val, &input->float_val, sizeof(uint32_t));
			memcpy(&field_double_val, &input->double_val, sizeof(uint64_t));
			memcpy(&field_short_val, &input->short_val, sizeof(uint16_t));
			memcpy(&field_int16_val, &input->int16_val, sizeof(uint16_t));
			memcpy(&field_long_val, &input->long_val, sizeof(uint32_t));
			memcpy(&field_int32_val, &input->int32_val, sizeof(uint32_t));
			memcpy(&field_long_long_val, &input->long_long_val, sizeof(uint64_t));
			memcpy(&field_int64_val, &input->int64_val, sizeof(uint64_t));
			memcpy(&field_unsigned_short_val, &input->unsigned_short_val, sizeof(uint16_t));
			memcpy(&field_uint16_val, &input->uint16_val, sizeof(uint16_t));
			memcpy(&field_unsigned_long_val, &input->unsigned_long_val, sizeof(uint32_t));
			memcpy(&field_uint32_val, &input->uint32_val, sizeof(uint32_t));
			memcpy(&field_unsigned_long_long_val, &input->unsigned_long_long_val, sizeof(uint64_t));
			memcpy(&field_uint64_val, &input->uint64_val, sizeof(uint64_t));
			memcpy(&field_char_val, &input->char_val, sizeof(uint8_t));
			memcpy(&field_int8_val, &input->int8_val, sizeof(uint8_t));
			memcpy(&field_bool_val, &input->bool_val, sizeof(uint8_t));
			memcpy(&field_octet_val, &input->octet_val, sizeof(uint8_t));
			memcpy(&field_uint8_val, &input->uint8_val, sizeof(uint8_t));
			field_float_val = htobe32(field_float_val);
			field_double_val = htobe64(field_double_val);
			field_short_val = htobe16(field_short_val);
			field_int16_val = htobe16(field_int16_val);
			field_long_val = htobe32(field_long_val);
			field_int32_val = htobe32(field_int32_val);
			field_long_long_val = htobe64(field_long_long_val);
			field_int64_val = htobe64(field_int64_val);
			field_unsigned_short_val = htobe16(field_unsigned_short_val);
			field_uint16_val = htobe16(field_uint16_val);
			field_unsigned_long_val = htobe32(field_unsigned_long_val);
			field_uint32_val = htobe32(field_uint32_val);
			field_unsigned_long_long_val = htobe64(field_unsigned_long_long_val);
			field_uint64_val = htobe64(field_uint64_val);
			memcpy(&output->float_val, &field_float_val, sizeof(uint32_t));
			memcpy(&output->double_val, &field_double_val, sizeof(uint64_t));
			memcpy(&output->short_val, &field_short_val, sizeof(uint16_t));
			memcpy(&output->int16_val, &field_int16_val, sizeof(uint16_t));
			memcpy(&output->long_val, &field_long_val, sizeof(uint32_t));
			memcpy(&output->int32_val, &field_int32_val, sizeof(uint32_t));
			memcpy(&output->long_long_val, &field_long_long_val, sizeof(uint64_t));
			memcpy(&output->int64_val, &field_int64_val, sizeof(uint64_t));
			memcpy(&output->unsigned_short_val, &field_unsigned_short_val, sizeof(uint16_t));
			memcpy(&output->uint16_val, &field_uint16_val, sizeof(uint16_t));
			memcpy(&output->unsigned_long_val, &field_unsigned_long_val, sizeof(uint32_t));
			memcpy(&output->uint32_val, &field_uint32_val, sizeof(uint32_t));
			memcpy(&output->unsigned_long_long_val, &field_unsigned_long_long_val, sizeof(uint64_t));
			memcpy(&output->uint64_val, &field_uint64_val, sizeof(uint64_t));
			memcpy(&output->char_val, &field_char_val, sizeof(uint8_t));
			memcpy(&output->int8_val, &field_int8_val, sizeof(uint8_t));
			memcpy(&output->bool_val, &field_bool_val, sizeof(uint8_t));
			memcpy(&output->octet_val, &field_octet_val, sizeof(uint8_t));
			memcpy(&output->uint8_val, &field_uint8_val, sizeof(uint8_t));
		}

		static struct primitives::primitives fromBuffer(std::vector<char> const& buf) {
			struct primitives::primitives retval;
			const struct primitives::primitives_wire* input = (const struct primitives::primitives_wire*) buf.data();
			struct primitives::primitives* output = &retval;
			if (buf.size() != sizeof(struct primitives::primitives)) {
				static const std::string error_msg =
					std::string("pirate::Serialization::fromBuffer() for primitives::primitives type did not receive a buffer of size ") +
					std::to_string(sizeof(struct primitives::primitives));
				throw std::length_error(error_msg);
			}
			uint32_t field_float_val;
			uint64_t field_double_val;
			uint16_t field_short_val;
			uint16_t field_int16_val;
			uint32_t field_long_val;
			uint32_t field_int32_val;
			uint64_t field_long_long_val;
			uint64_t field_int64_val;
			uint16_t field_unsigned_short_val;
			uint16_t field_uint16_val;
			uint32_t field_unsigned_long_val;
			uint32_t field_uint32_val;
			uint64_t field_unsigned_long_long_val;
			uint64_t field_uint64_val;
			uint8_t field_char_val;
			uint8_t field_int8_val;
			uint8_t field_bool_val;
			uint8_t field_octet_val;
			uint8_t field_uint8_val;
			memcpy(&field_float_val, &input->float_val, sizeof(uint32_t));
			memcpy(&field_double_val, &input->double_val, sizeof(uint64_t));
			memcpy(&field_short_val, &input->short_val, sizeof(uint16_t));
			memcpy(&field_int16_val, &input->int16_val, sizeof(uint16_t));
			memcpy(&field_long_val, &input->long_val, sizeof(uint32_t));
			memcpy(&field_int32_val, &input->int32_val, sizeof(uint32_t));
			memcpy(&field_long_long_val, &input->long_long_val, sizeof(uint64_t));
			memcpy(&field_int64_val, &input->int64_val, sizeof(uint64_t));
			memcpy(&field_unsigned_short_val, &input->unsigned_short_val, sizeof(uint16_t));
			memcpy(&field_uint16_val, &input->uint16_val, sizeof(uint16_t));
			memcpy(&field_unsigned_long_val, &input->unsigned_long_val, sizeof(uint32_t));
			memcpy(&field_uint32_val, &input->uint32_val, sizeof(uint32_t));
			memcpy(&field_unsigned_long_long_val, &input->unsigned_long_long_val, sizeof(uint64_t));
			memcpy(&field_uint64_val, &input->uint64_val, sizeof(uint64_t));
			memcpy(&field_char_val, &input->char_val, sizeof(uint8_t));
			memcpy(&field_int8_val, &input->int8_val, sizeof(uint8_t));
			memcpy(&field_bool_val, &input->bool_val, sizeof(uint8_t));
			memcpy(&field_octet_val, &input->octet_val, sizeof(uint8_t));
			memcpy(&field_uint8_val, &input->uint8_val, sizeof(uint8_t));
			field_float_val = be32toh(field_float_val);
			field_double_val = be64toh(field_double_val);
			field_short_val = be16toh(field_short_val);
			field_int16_val = be16toh(field_int16_val);
			field_long_val = be32toh(field_long_val);
			field_int32_val = be32toh(field_int32_val);
			field_long_long_val = be64toh(field_long_long_val);
			field_int64_val = be64toh(field_int64_val);
			field_unsigned_short_val = be16toh(field_unsigned_short_val);
			field_uint16_val = be16toh(field_uint16_val);
			field_unsigned_long_val = be32toh(field_unsigned_long_val);
			field_uint32_val = be32toh(field_uint32_val);
			field_unsigned_long_long_val = be64toh(field_unsigned_long_long_val);
			field_uint64_val = be64toh(field_uint64_val);
			memcpy(&output->float_val, &field_float_val, sizeof(uint32_t));
			memcpy(&output->double_val, &field_double_val, sizeof(uint64_t));
			memcpy(&output->short_val, &field_short_val, sizeof(uint16_t));
			memcpy(&output->int16_val, &field_int16_val, sizeof(uint16_t));
			memcpy(&output->long_val, &field_long_val, sizeof(uint32_t));
			memcpy(&output->int32_val, &field_int32_val, sizeof(uint32_t));
			memcpy(&output->long_long_val, &field_long_long_val, sizeof(uint64_t));
			memcpy(&output->int64_val, &field_int64_val, sizeof(uint64_t));
			memcpy(&output->unsigned_short_val, &field_unsigned_short_val, sizeof(uint16_t));
			memcpy(&output->uint16_val, &field_uint16_val, sizeof(uint16_t));
			memcpy(&output->unsigned_long_val, &field_unsigned_long_val, sizeof(uint32_t));
			memcpy(&output->uint32_val, &field_uint32_val, sizeof(uint32_t));
			memcpy(&output->unsigned_long_long_val, &field_unsigned_long_long_val, sizeof(uint64_t));
			memcpy(&output->uint64_val, &field_uint64_val, sizeof(uint64_t));
			memcpy(&output->char_val, &field_char_val, sizeof(uint8_t));
			memcpy(&output->int8_val, &field_int8_val, sizeof(uint8_t));
			memcpy(&output->bool_val, &field_bool_val, sizeof(uint8_t));
			memcpy(&output->octet_val, &field_octet_val, sizeof(uint8_t));
			memcpy(&output->uint8_val, &field_uint8_val, sizeof(uint8_t));
			return retval;
		}
	};
}
