#ifndef _UNIONTYPE_IDL_CODEGEN_H
#define _UNIONTYPE_IDL_CODEGEN_H

#include <cassert>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

#include <endian.h>

namespace UnionType {

	struct Union_Example {
		int16_t tag __attribute__((aligned(2)));
		union {
			uint8_t a __attribute__((aligned(1)));
			int32_t b __attribute__((aligned(4)));
			float c __attribute__((aligned(4)));
		} data;
	};

	struct Union_Example_wire {
		unsigned char tag[2];
		union {
			unsigned char a[1];
			unsigned char b[4];
			unsigned char c[4];
		} data;
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

	template<>
	struct Serialization<struct UnionType::Union_Example> {
		static void toBuffer(struct UnionType::Union_Example const& val, std::vector<char>& buf) {
			buf.resize(sizeof(struct UnionType::Union_Example));
			struct UnionType::Union_Example_wire* output = (struct UnionType::Union_Example_wire*) buf.data();
			const struct UnionType::Union_Example* input = &val;
			uint16_t tag;
			uint8_t data_a;
			uint32_t data_b;
			uint32_t data_c;
			memset(output, 0, sizeof(*output));
			memcpy(&tag, &input->tag, sizeof(uint16_t));
			tag = htobe16(tag);
			memcpy(&output->tag, &tag, sizeof(uint16_t));
			switch (input->tag) {
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

		static struct UnionType::Union_Example fromBuffer(std::vector<char> const& buf) {
			struct UnionType::Union_Example retval;
			const struct UnionType::Union_Example_wire* input = (const struct UnionType::Union_Example_wire*) buf.data();
			struct UnionType::Union_Example* output = &retval;
			if (buf.size() != sizeof(struct UnionType::Union_Example)) {
				static const std::string error_msg =
					std::string("pirate::Serialization::fromBuffer() for UnionType::Union_Example type did not receive a buffer of size ") +
					std::to_string(sizeof(struct UnionType::Union_Example));
				throw std::length_error(error_msg);
			}
			uint16_t tag;
			uint8_t data_a;
			uint32_t data_b;
			uint32_t data_c;
			memcpy(&tag, &input->tag, sizeof(uint16_t));
			tag = be16toh(tag);
			memcpy(&output->tag, &tag, sizeof(uint16_t));
			switch (output->tag) {
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
			return retval;
		}
	};
}

#endif // _UNIONTYPE_IDL_CODEGEN_H
