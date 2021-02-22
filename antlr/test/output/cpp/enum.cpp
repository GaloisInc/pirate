#ifndef _ENUMTYPE_IDL_CODEGEN_H
#define _ENUMTYPE_IDL_CODEGEN_H

#include <cassert>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

#include <endian.h>

namespace EnumType {

	enum class DayOfWeek : uint32_t {
		Monday,
		Tuesday,
		Wednesday,
		Thursday,
		Friday
	};

	struct Week_Interval {
		DayOfWeek begin __attribute__((aligned(4)));
		DayOfWeek end __attribute__((aligned(4)));
	};

	struct Week_Interval_wire {
		unsigned char begin[4] __attribute__((aligned(4)));
		unsigned char end[4] __attribute__((aligned(4)));
	};

	static_assert(sizeof(struct Week_Interval) == sizeof(struct Week_Interval_wire), "size of struct Week_Interval not equal to wire protocol struct");
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

	inline void toWireType(const struct EnumType::Week_Interval* input, struct EnumType::Week_Interval_wire* output) {
		uint32_t field_begin;
		uint32_t field_end;
		memset(output, 0, sizeof(*output));
		memcpy(&field_begin, &input->begin, sizeof(uint32_t));
		memcpy(&field_end, &input->end, sizeof(uint32_t));
		field_begin = htobe32(field_begin);
		field_end = htobe32(field_end);
		memcpy(&output->begin, &field_begin, sizeof(uint32_t));
		memcpy(&output->end, &field_end, sizeof(uint32_t));
	}

	inline struct EnumType::Week_Interval fromWireType(const struct EnumType::Week_Interval_wire* input) {
		struct EnumType::Week_Interval retval;
		struct EnumType::Week_Interval* output = &retval;
		uint32_t field_begin;
		uint32_t field_end;
		memcpy(&field_begin, &input->begin, sizeof(uint32_t));
		memcpy(&field_end, &input->end, sizeof(uint32_t));
		field_begin = be32toh(field_begin);
		field_end = be32toh(field_end);
		memcpy(&output->begin, &field_begin, sizeof(uint32_t));
		memcpy(&output->end, &field_end, sizeof(uint32_t));
		return retval;
	}

	template<>
	struct Serialization<struct EnumType::Week_Interval> {
		static void toBuffer(struct EnumType::Week_Interval const& val, std::vector<char>& buf) {
			buf.resize(sizeof(struct EnumType::Week_Interval_wire));
			struct EnumType::Week_Interval_wire* output = (struct EnumType::Week_Interval_wire*) buf.data();
			const struct EnumType::Week_Interval* input = &val;
			toWireType(input, output);
		}

		static struct EnumType::Week_Interval fromBuffer(std::vector<char> const& buf) {
			const struct EnumType::Week_Interval_wire* input = (const struct EnumType::Week_Interval_wire*) buf.data();
			if (buf.size() != sizeof(struct EnumType::Week_Interval_wire)) {
				static const std::string error_msg =
					std::string("pirate::Serialization::fromBuffer() for EnumType::Week_Interval type did not receive a buffer of size ") +
					std::to_string(sizeof(struct EnumType::Week_Interval_wire));
				throw std::length_error(error_msg);
			}
			return fromWireType(input);
		}
	};
}

#endif // _ENUMTYPE_IDL_CODEGEN_H
