#include <cassert>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

#include <endian.h>

namespace annotations_module {

	struct annotation_struct_example {
		double x __attribute__((aligned(8)));
		double y __attribute__((aligned(8)));
		double z __attribute__((aligned(8)));
	};

	struct annotation_union_example {
		int16_t tag __attribute__((aligned(2)));
		union {
			int16_t a __attribute__((aligned(2)));
			int32_t b __attribute__((aligned(4)));
			float c __attribute__((aligned(4)));
		} data;
	};

	struct annotation_struct_example_wire {
		unsigned char x[8] __attribute__((aligned(8)));
		unsigned char y[8] __attribute__((aligned(8)));
		unsigned char z[8] __attribute__((aligned(8)));
	};

	struct annotation_union_example_wire {
		unsigned char tag[2];
		union {
			unsigned char a[2] __attribute__((aligned(2)));
			unsigned char b[4] __attribute__((aligned(4)));
			unsigned char c[4] __attribute__((aligned(4)));
		} data;
	};

	static_assert(sizeof(struct annotation_struct_example) == sizeof(struct annotation_struct_example_wire), "size of struct annotation_struct_example not equal to wire protocol struct");
	static_assert(sizeof(struct annotation_union_example) == sizeof(struct annotation_union_example_wire), "size of annotation_union_example not equal to wire protocol size"
	);
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
	struct Serialization<struct annotations_module::annotation_struct_example> {
		static void toBuffer(struct annotations_module::annotation_struct_example const& val, std::vector<char>& buf) {
			buf.resize(sizeof(struct annotations_module::annotation_struct_example));
			struct annotations_module::annotation_struct_example_wire* output = (struct annotations_module::annotation_struct_example_wire*) buf.data();
			const struct annotations_module::annotation_struct_example* input = &val;
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

		static struct annotations_module::annotation_struct_example fromBuffer(std::vector<char> const& buf) {
			struct annotations_module::annotation_struct_example retval;
			const struct annotations_module::annotation_struct_example_wire* input = (const struct annotations_module::annotation_struct_example_wire*) buf.data();
			struct annotations_module::annotation_struct_example* output = &retval;
			if (buf.size() != sizeof(struct annotations_module::annotation_struct_example)) {
				static const std::string error_msg =
					std::string("pirate::Serialization::fromBuffer() for annotations_module::annotation_struct_example type did not receive a buffer of size ") +
					std::to_string(sizeof(struct annotations_module::annotation_struct_example));
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
	struct Serialization<struct annotations_module::annotation_union_example> {
		static void toBuffer(struct annotations_module::annotation_union_example const& val, std::vector<char>& buf) {
			buf.resize(sizeof(struct annotations_module::annotation_union_example));
			struct annotations_module::annotation_union_example_wire* output = (struct annotations_module::annotation_union_example_wire*) buf.data();
			const struct annotations_module::annotation_union_example* input = &val;
			uint16_t tag;
			uint16_t data_a;
			uint32_t data_b;
			uint32_t data_c;
			memcpy(&tag, &input->tag, sizeof(uint16_t));
			tag = htobe16(tag);
			memcpy(&output->tag, &tag, sizeof(uint16_t));
			switch (input->tag) {
			case 1:
				memcpy(&data_a, &input->data.a, sizeof(uint16_t));
				data_a = htobe16(data_a);
				memcpy(&output->data.a, &data_a, sizeof(uint16_t));
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

		static struct annotations_module::annotation_union_example fromBuffer(std::vector<char> const& buf) {
			struct annotations_module::annotation_union_example retval;
			const struct annotations_module::annotation_union_example_wire* input = (const struct annotations_module::annotation_union_example_wire*) buf.data();
			struct annotations_module::annotation_union_example* output = &retval;
			if (buf.size() != sizeof(struct annotations_module::annotation_union_example)) {
				static const std::string error_msg =
					std::string("pirate::Serialization::fromBuffer() for annotations_module::annotation_union_example type did not receive a buffer of size ") +
					std::to_string(sizeof(struct annotations_module::annotation_union_example));
				throw std::length_error(error_msg);
			}
			uint16_t tag;
			uint16_t data_a;
			uint32_t data_b;
			uint32_t data_c;
			memcpy(&tag, &input->tag, sizeof(uint16_t));
			tag = be16toh(tag);
			memcpy(&output->tag, &tag, sizeof(uint16_t));
			switch (output->tag) {
			case 1:
				memcpy(&data_a, &input->data.a, sizeof(uint16_t));
				data_a = be16toh(data_a);
				memcpy(&output->data.a, &data_a, sizeof(uint16_t));
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
