#ifndef _ARRAYS_IDL_CODEGEN_H
#define _ARRAYS_IDL_CODEGEN_H

#include <cassert>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

#include <endian.h>

namespace Arrays {

	struct Union_Array_Field {
		int16_t tag __attribute__((aligned(2)));
		union {
			uint8_t a __attribute__((aligned(1)));
			int32_t b[10] __attribute__((aligned(4)));
			float c[1][2][3] __attribute__((aligned(4)));
		} data;
	};

	struct Struct_Array_Field {
		uint8_t a __attribute__((aligned(1)));
		int32_t b[10] __attribute__((aligned(4)));
		float c[1][2][3][4][5][6] __attribute__((aligned(4)));
	};

	struct Union_Array_Field_wire {
		unsigned char tag[2];
		union {
			unsigned char a[1] __attribute__((aligned(1)));
			unsigned char b[10][4] __attribute__((aligned(4)));
			unsigned char c[1][2][3][4] __attribute__((aligned(4)));
		} data;
	};

	struct Struct_Array_Field_wire {
		unsigned char a[1] __attribute__((aligned(1)));
		unsigned char b[10][4] __attribute__((aligned(4)));
		unsigned char c[1][2][3][4][5][6][4] __attribute__((aligned(4)));
	};

	static_assert(sizeof(struct Union_Array_Field) == sizeof(struct Union_Array_Field_wire), "size of Union_Array_Field not equal to wire protocol size"
	);
	static_assert(sizeof(struct Struct_Array_Field) == sizeof(struct Struct_Array_Field_wire), "size of struct Struct_Array_Field not equal to wire protocol struct");
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

	inline void toWireType(const struct Arrays::Union_Array_Field* input, struct Arrays::Union_Array_Field_wire* output) {
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
			for (size_t b_0 = 0; b_0 < 10; b_0++) {
				memcpy(&data_b, &input->data.b[b_0], sizeof(uint32_t));
				data_b = htobe32(data_b);
				memcpy(&output->data.b[b_0], &data_b, sizeof(uint32_t));
			}
			break;
		case 4:
		default:
			for (size_t c_0 = 0; c_0 < 1; c_0++) {
				for (size_t c_1 = 0; c_1 < 2; c_1++) {
					for (size_t c_2 = 0; c_2 < 3; c_2++) {
						memcpy(&data_c, &input->data.c[c_0][c_1][c_2], sizeof(uint32_t));
						data_c = htobe32(data_c);
						memcpy(&output->data.c[c_0][c_1][c_2], &data_c, sizeof(uint32_t));
					}
				}
			}
			break;
		}
	}

	inline struct Arrays::Union_Array_Field fromWireType(const struct Arrays::Union_Array_Field_wire* input) {
		struct Arrays::Union_Array_Field retval;
		struct Arrays::Union_Array_Field* output = &retval;
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
			for (size_t b_0 = 0; b_0 < 10; b_0++) {
				memcpy(&data_b, &input->data.b[b_0], sizeof(uint32_t));
				data_b = be32toh(data_b);
				memcpy(&output->data.b[b_0], &data_b, sizeof(uint32_t));
			}
			break;
		case 4:
		default:
			for (size_t c_0 = 0; c_0 < 1; c_0++) {
				for (size_t c_1 = 0; c_1 < 2; c_1++) {
					for (size_t c_2 = 0; c_2 < 3; c_2++) {
						memcpy(&data_c, &input->data.c[c_0][c_1][c_2], sizeof(uint32_t));
						data_c = be32toh(data_c);
						memcpy(&output->data.c[c_0][c_1][c_2], &data_c, sizeof(uint32_t));
					}
				}
			}
			break;
		}
		return retval;
	}

	template<>
	struct Serialization<struct Arrays::Union_Array_Field> {
		static void toBuffer(struct Arrays::Union_Array_Field const& val, std::vector<char>& buf) {
			buf.resize(sizeof(struct Arrays::Union_Array_Field));
			struct Arrays::Union_Array_Field_wire* output = (struct Arrays::Union_Array_Field_wire*) buf.data();
			const struct Arrays::Union_Array_Field* input = &val;
			toWireType(input, output);
		}

		static struct Arrays::Union_Array_Field fromBuffer(std::vector<char> const& buf) {
			const struct Arrays::Union_Array_Field_wire* input = (const struct Arrays::Union_Array_Field_wire*) buf.data();
			if (buf.size() != sizeof(struct Arrays::Union_Array_Field)) {
				static const std::string error_msg =
					std::string("pirate::Serialization::fromBuffer() for Arrays::Union_Array_Field type did not receive a buffer of size ") +
					std::to_string(sizeof(struct Arrays::Union_Array_Field));
				throw std::length_error(error_msg);
			}
			return fromWireType(input);
		}
	};

	inline void toWireType(const struct Arrays::Struct_Array_Field* input, struct Arrays::Struct_Array_Field_wire* output) {
		uint8_t field_a;
		uint32_t field_b;
		uint32_t field_c;
		memset(output, 0, sizeof(*output));
		for (size_t b_0 = 0; b_0 < 10; b_0++) {
			memcpy(&field_b, &input->b[b_0], sizeof(uint32_t));
			field_b = htobe32(field_b);
			memcpy(&output->b[b_0], &field_b, sizeof(uint32_t));
		}
		for (size_t c_0 = 0; c_0 < 1; c_0++) {
			for (size_t c_1 = 0; c_1 < 2; c_1++) {
				for (size_t c_2 = 0; c_2 < 3; c_2++) {
					for (size_t c_3 = 0; c_3 < 4; c_3++) {
						for (size_t c_4 = 0; c_4 < 5; c_4++) {
							for (size_t c_5 = 0; c_5 < 6; c_5++) {
								memcpy(&field_c, &input->c[c_0][c_1][c_2][c_3][c_4][c_5], sizeof(uint32_t));
								field_c = htobe32(field_c);
								memcpy(&output->c[c_0][c_1][c_2][c_3][c_4][c_5], &field_c, sizeof(uint32_t));
							}
						}
					}
				}
			}
		}
		memcpy(&field_a, &input->a, sizeof(uint8_t));
		memcpy(&output->a, &field_a, sizeof(uint8_t));
	}

	inline struct Arrays::Struct_Array_Field fromWireType(const struct Arrays::Struct_Array_Field_wire* input) {
		struct Arrays::Struct_Array_Field retval;
		struct Arrays::Struct_Array_Field* output = &retval;
		uint8_t field_a;
		uint32_t field_b;
		uint32_t field_c;
		for (size_t b_0 = 0; b_0 < 10; b_0++) {
			memcpy(&field_b, &input->b[b_0], sizeof(uint32_t));
			field_b = be32toh(field_b);
			memcpy(&output->b[b_0], &field_b, sizeof(uint32_t));
		}
		for (size_t c_0 = 0; c_0 < 1; c_0++) {
			for (size_t c_1 = 0; c_1 < 2; c_1++) {
				for (size_t c_2 = 0; c_2 < 3; c_2++) {
					for (size_t c_3 = 0; c_3 < 4; c_3++) {
						for (size_t c_4 = 0; c_4 < 5; c_4++) {
							for (size_t c_5 = 0; c_5 < 6; c_5++) {
								memcpy(&field_c, &input->c[c_0][c_1][c_2][c_3][c_4][c_5], sizeof(uint32_t));
								field_c = be32toh(field_c);
								memcpy(&output->c[c_0][c_1][c_2][c_3][c_4][c_5], &field_c, sizeof(uint32_t));
							}
						}
					}
				}
			}
		}
		memcpy(&field_a, &input->a, sizeof(uint8_t));
		memcpy(&output->a, &field_a, sizeof(uint8_t));
		return retval;
	}

	template<>
	struct Serialization<struct Arrays::Struct_Array_Field> {
		static void toBuffer(struct Arrays::Struct_Array_Field const& val, std::vector<char>& buf) {
			buf.resize(sizeof(struct Arrays::Struct_Array_Field));
			struct Arrays::Struct_Array_Field_wire* output = (struct Arrays::Struct_Array_Field_wire*) buf.data();
			const struct Arrays::Struct_Array_Field* input = &val;
			toWireType(input, output);
		}

		static struct Arrays::Struct_Array_Field fromBuffer(std::vector<char> const& buf) {
			const struct Arrays::Struct_Array_Field_wire* input = (const struct Arrays::Struct_Array_Field_wire*) buf.data();
			if (buf.size() != sizeof(struct Arrays::Struct_Array_Field)) {
				static const std::string error_msg =
					std::string("pirate::Serialization::fromBuffer() for Arrays::Struct_Array_Field type did not receive a buffer of size ") +
					std::to_string(sizeof(struct Arrays::Struct_Array_Field));
				throw std::length_error(error_msg);
			}
			return fromWireType(input);
		}
	};
}

#endif // _ARRAYS_IDL_CODEGEN_H
