#include <cassert>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

#include <endian.h>

namespace pnt {

	struct position {
		double x __attribute__((aligned(8)));
		double y __attribute__((aligned(8)));
		double z __attribute__((aligned(8)));
	};

	struct distance {
		double x __attribute__((aligned(8)));
		double y __attribute__((aligned(8)));
		double z __attribute__((aligned(8)));
	};

	struct position_wire {
		unsigned char x[8] __attribute__((aligned(8)));
		unsigned char y[8] __attribute__((aligned(8)));
		unsigned char z[8] __attribute__((aligned(8)));
	};

	struct distance_wire {
		unsigned char x[8] __attribute__((aligned(8)));
		unsigned char y[8] __attribute__((aligned(8)));
		unsigned char z[8] __attribute__((aligned(8)));
	};

	static_assert(sizeof(struct position) == sizeof(struct position_wire), "size of struct position not equal to wire protocol struct");
	static_assert(sizeof(struct distance) == sizeof(struct distance_wire), "size of struct distance not equal to wire protocol struct");
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
	struct Serialization<struct pnt::position> {
		static void toBuffer(struct pnt::position const& val, std::vector<char>& buf) {
			buf.resize(sizeof(struct pnt::position));
			struct pnt::position_wire* output = (struct pnt::position_wire*) buf.data();
			const struct pnt::position* input = &val;
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

		static struct pnt::position fromBuffer(std::vector<char> const& buf) {
			struct pnt::position retval;
			const struct pnt::position_wire* input = (const struct pnt::position_wire*) buf.data();
			struct pnt::position* output = &retval;
			if (buf.size() != sizeof(struct pnt::position)) {
				static const std::string error_msg =
					std::string("pirate::Serialization::fromBuffer() for pnt::position type did not receive a buffer of size ") +
					std::to_string(sizeof(struct pnt::position));
				throw std::length_error(error_msg);
			}
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
			return retval;
		}
	};

	template<>
	struct Serialization<struct pnt::distance> {
		static void toBuffer(struct pnt::distance const& val, std::vector<char>& buf) {
			buf.resize(sizeof(struct pnt::distance));
			struct pnt::distance_wire* output = (struct pnt::distance_wire*) buf.data();
			const struct pnt::distance* input = &val;
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

		static struct pnt::distance fromBuffer(std::vector<char> const& buf) {
			struct pnt::distance retval;
			const struct pnt::distance_wire* input = (const struct pnt::distance_wire*) buf.data();
			struct pnt::distance* output = &retval;
			if (buf.size() != sizeof(struct pnt::distance)) {
				static const std::string error_msg =
					std::string("pirate::Serialization::fromBuffer() for pnt::distance type did not receive a buffer of size ") +
					std::to_string(sizeof(struct pnt::distance));
				throw std::length_error(error_msg);
			}
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
			return retval;
		}
	};
}
