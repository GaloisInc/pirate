#ifndef _PNT_IDL_CODEGEN_H
#define _PNT_IDL_CODEGEN_H

#include <cassert>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

#include <endian.h>

namespace PNT {

	struct Position {
		double x __attribute__((aligned(8)));
		double y __attribute__((aligned(8)));
		double z __attribute__((aligned(8)));
	};

	struct Distance {
		double x __attribute__((aligned(8)));
		double y __attribute__((aligned(8)));
		double z __attribute__((aligned(8)));
	};

	struct Position_wire {
		unsigned char x[8];
		unsigned char y[8];
		unsigned char z[8];
	} __attribute__((packed)) ;

	struct Distance_wire {
		unsigned char x[8];
		unsigned char y[8];
		unsigned char z[8];
	} __attribute__((packed)) ;

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

	inline void toWireType(const struct PNT::Position* input, struct PNT::Position_wire* output) {
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

	inline struct PNT::Position fromWireType(const struct PNT::Position_wire* input) {
		struct PNT::Position retval;
		struct PNT::Position* output = &retval;
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

	template<>
	struct Serialization<struct PNT::Position> {
		static void toBuffer(struct PNT::Position const& val, std::vector<char>& buf) {
			buf.resize(sizeof(struct PNT::Position_wire));
			struct PNT::Position_wire* output = (struct PNT::Position_wire*) buf.data();
			const struct PNT::Position* input = &val;
			toWireType(input, output);
		}

		static struct PNT::Position fromBuffer(std::vector<char> const& buf) {
			const struct PNT::Position_wire* input = (const struct PNT::Position_wire*) buf.data();
			if (buf.size() != sizeof(struct PNT::Position_wire)) {
				static const std::string error_msg =
					std::string("pirate::Serialization::fromBuffer() for PNT::Position type did not receive a buffer of size ") +
					std::to_string(sizeof(struct PNT::Position_wire));
				throw std::length_error(error_msg);
			}
			return fromWireType(input);
		}
	};

	inline void toWireType(const struct PNT::Distance* input, struct PNT::Distance_wire* output) {
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

	inline struct PNT::Distance fromWireType(const struct PNT::Distance_wire* input) {
		struct PNT::Distance retval;
		struct PNT::Distance* output = &retval;
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

	template<>
	struct Serialization<struct PNT::Distance> {
		static void toBuffer(struct PNT::Distance const& val, std::vector<char>& buf) {
			buf.resize(sizeof(struct PNT::Distance_wire));
			struct PNT::Distance_wire* output = (struct PNT::Distance_wire*) buf.data();
			const struct PNT::Distance* input = &val;
			toWireType(input, output);
		}

		static struct PNT::Distance fromBuffer(std::vector<char> const& buf) {
			const struct PNT::Distance_wire* input = (const struct PNT::Distance_wire*) buf.data();
			if (buf.size() != sizeof(struct PNT::Distance_wire)) {
				static const std::string error_msg =
					std::string("pirate::Serialization::fromBuffer() for PNT::Distance type did not receive a buffer of size ") +
					std::to_string(sizeof(struct PNT::Distance_wire));
				throw std::length_error(error_msg);
			}
			return fromWireType(input);
		}
	};
}

#endif // _PNT_IDL_CODEGEN_H
