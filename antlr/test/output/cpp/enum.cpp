#include <cassert>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

#include <endian.h>

namespace enumtype {

	enum class DayOfWeek : uint32_t {
		Monday,
		Tuesday,
		Wednesday,
		Thursday,
		Friday
	};

	struct week_interval {
		DayOfWeek begin __attribute__((aligned(4)));
		DayOfWeek end __attribute__((aligned(4)));
	};

	struct week_interval_wire {
		unsigned char begin[4] __attribute__((aligned(4)));
		unsigned char end[4] __attribute__((aligned(4)));
	};

	static_assert(sizeof(struct week_interval) == sizeof(struct week_interval_wire), "size of struct week_interval not equal to wire protocol struct");
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
	struct Serialization<struct enumtype::week_interval> {
		static void toBuffer(struct enumtype::week_interval const& val, std::vector<char>& buf) {
			buf.resize(sizeof(struct enumtype::week_interval));
			struct enumtype::week_interval_wire* output = (struct enumtype::week_interval_wire*) buf.data();
			const struct enumtype::week_interval* input = &val;
			uint32_t field_begin;
			uint32_t field_end;
			memcpy(&field_begin, &input->begin, sizeof(uint32_t));
			memcpy(&field_end, &input->end, sizeof(uint32_t));
			field_begin = htobe32(field_begin);
			field_end = htobe32(field_end);
			memcpy(&output->begin, &field_begin, sizeof(uint32_t));
			memcpy(&output->end, &field_end, sizeof(uint32_t));
		}

		static struct enumtype::week_interval fromBuffer(std::vector<char> const& buf) {
			struct enumtype::week_interval retval;
			const struct enumtype::week_interval_wire* input = (const struct enumtype::week_interval_wire*) buf.data();
			struct enumtype::week_interval* output = &retval;
			if (buf.size() != sizeof(struct enumtype::week_interval)) {
				static const std::string error_msg =
					std::string("pirate::Serialization::fromBuffer() for enumtype::week_interval type did not receive a buffer of size ") +
					std::to_string(sizeof(struct enumtype::week_interval));
				throw std::length_error(error_msg);
			}
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
	};
}
