#ifndef _NESTEDTYPES_IDL_CODEGEN_H
#define _NESTEDTYPES_IDL_CODEGEN_H

#include <cassert>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

#include <endian.h>

namespace NestedTypes {

	struct Foo {
		int32_t a __attribute__((aligned(4)));
		int32_t b __attribute__((aligned(4)));
		int32_t c __attribute__((aligned(4)));
	};

	struct Bar {
		double x __attribute__((aligned(8)));
		double y __attribute__((aligned(8)));
		double z __attribute__((aligned(8)));
	};

	struct OuterStruct {
		struct Foo foo;
		struct Bar bar;
	};

	enum class DayOfWeek : uint32_t {
		Monday,
		Tuesday,
		Wednesday,
		Thursday,
		Friday
	};

	struct OuterUnion {
		DayOfWeek tag __attribute__((aligned(4)));
		union {
			struct Foo foo;
			struct Bar bar;
		} data;
	};

	struct Foo_wire {
		unsigned char a[4];
		unsigned char b[4];
		unsigned char c[4];
	} __attribute__((packed)) ;

	struct Bar_wire {
		unsigned char x[8];
		unsigned char y[8];
		unsigned char z[8];
	} __attribute__((packed)) ;

	struct OuterStruct_wire {
		struct Foo_wire foo;
		struct Bar_wire bar;
	} __attribute__((packed)) ;

	struct OuterUnion_wire {
		unsigned char tag[4];
		union {
			struct Foo_wire foo;
			struct Bar_wire bar;
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

	inline void toWireType(const struct NestedTypes::Foo* input, struct NestedTypes::Foo_wire* output) {
		uint32_t field_a;
		uint32_t field_b;
		uint32_t field_c;
		memcpy(&field_a, &input->a, sizeof(uint32_t));
		memcpy(&field_b, &input->b, sizeof(uint32_t));
		memcpy(&field_c, &input->c, sizeof(uint32_t));
		field_a = htobe32(field_a);
		field_b = htobe32(field_b);
		field_c = htobe32(field_c);
		memcpy(&output->a, &field_a, sizeof(uint32_t));
		memcpy(&output->b, &field_b, sizeof(uint32_t));
		memcpy(&output->c, &field_c, sizeof(uint32_t));
	}

	inline struct NestedTypes::Foo fromWireType(const struct NestedTypes::Foo_wire* input) {
		struct NestedTypes::Foo retval;
		struct NestedTypes::Foo* output = &retval;
		uint32_t field_a;
		uint32_t field_b;
		uint32_t field_c;
		memcpy(&field_a, &input->a, sizeof(uint32_t));
		memcpy(&field_b, &input->b, sizeof(uint32_t));
		memcpy(&field_c, &input->c, sizeof(uint32_t));
		field_a = be32toh(field_a);
		field_b = be32toh(field_b);
		field_c = be32toh(field_c);
		memcpy(&output->a, &field_a, sizeof(uint32_t));
		memcpy(&output->b, &field_b, sizeof(uint32_t));
		memcpy(&output->c, &field_c, sizeof(uint32_t));
		return retval;
	}

	template<>
	struct Serialization<struct NestedTypes::Foo> {
		static void toBuffer(struct NestedTypes::Foo const& val, std::vector<char>& buf) {
			buf.resize(sizeof(struct NestedTypes::Foo));
			struct NestedTypes::Foo_wire* output = (struct NestedTypes::Foo_wire*) buf.data();
			const struct NestedTypes::Foo* input = &val;
			toWireType(input, output);
		}

		static struct NestedTypes::Foo fromBuffer(std::vector<char> const& buf) {
			const struct NestedTypes::Foo_wire* input = (const struct NestedTypes::Foo_wire*) buf.data();
			if (buf.size() != sizeof(struct NestedTypes::Foo)) {
				static const std::string error_msg =
					std::string("pirate::Serialization::fromBuffer() for NestedTypes::Foo type did not receive a buffer of size ") +
					std::to_string(sizeof(struct NestedTypes::Foo));
				throw std::length_error(error_msg);
			}
			return fromWireType(input);
		}
	};

	inline void toWireType(const struct NestedTypes::Bar* input, struct NestedTypes::Bar_wire* output) {
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

	inline struct NestedTypes::Bar fromWireType(const struct NestedTypes::Bar_wire* input) {
		struct NestedTypes::Bar retval;
		struct NestedTypes::Bar* output = &retval;
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
	struct Serialization<struct NestedTypes::Bar> {
		static void toBuffer(struct NestedTypes::Bar const& val, std::vector<char>& buf) {
			buf.resize(sizeof(struct NestedTypes::Bar));
			struct NestedTypes::Bar_wire* output = (struct NestedTypes::Bar_wire*) buf.data();
			const struct NestedTypes::Bar* input = &val;
			toWireType(input, output);
		}

		static struct NestedTypes::Bar fromBuffer(std::vector<char> const& buf) {
			const struct NestedTypes::Bar_wire* input = (const struct NestedTypes::Bar_wire*) buf.data();
			if (buf.size() != sizeof(struct NestedTypes::Bar)) {
				static const std::string error_msg =
					std::string("pirate::Serialization::fromBuffer() for NestedTypes::Bar type did not receive a buffer of size ") +
					std::to_string(sizeof(struct NestedTypes::Bar));
				throw std::length_error(error_msg);
			}
			return fromWireType(input);
		}
	};

	inline void toWireType(const struct NestedTypes::OuterStruct* input, struct NestedTypes::OuterStruct_wire* output) {
		toWireType(&input->foo, &output->foo);
		toWireType(&input->bar, &output->bar);
	}

	inline struct NestedTypes::OuterStruct fromWireType(const struct NestedTypes::OuterStruct_wire* input) {
		struct NestedTypes::OuterStruct retval;
		struct NestedTypes::OuterStruct* output = &retval;
		output->foo = fromWireType(&input->foo);
		output->bar = fromWireType(&input->bar);
		return retval;
	}

	template<>
	struct Serialization<struct NestedTypes::OuterStruct> {
		static void toBuffer(struct NestedTypes::OuterStruct const& val, std::vector<char>& buf) {
			buf.resize(sizeof(struct NestedTypes::OuterStruct));
			struct NestedTypes::OuterStruct_wire* output = (struct NestedTypes::OuterStruct_wire*) buf.data();
			const struct NestedTypes::OuterStruct* input = &val;
			toWireType(input, output);
		}

		static struct NestedTypes::OuterStruct fromBuffer(std::vector<char> const& buf) {
			const struct NestedTypes::OuterStruct_wire* input = (const struct NestedTypes::OuterStruct_wire*) buf.data();
			if (buf.size() != sizeof(struct NestedTypes::OuterStruct)) {
				static const std::string error_msg =
					std::string("pirate::Serialization::fromBuffer() for NestedTypes::OuterStruct type did not receive a buffer of size ") +
					std::to_string(sizeof(struct NestedTypes::OuterStruct));
				throw std::length_error(error_msg);
			}
			return fromWireType(input);
		}
	};

	inline void toWireType(const struct NestedTypes::OuterUnion* input, struct NestedTypes::OuterUnion_wire* output) {
		uint32_t tag;
		memset(output, 0, sizeof(*output));
		memcpy(&tag, &input->tag, sizeof(uint32_t));
		tag = htobe32(tag);
		memcpy(&output->tag, &tag, sizeof(uint32_t));
		switch (input->tag) {
		case NestedTypes::DayOfWeek::Monday:
		case NestedTypes::DayOfWeek::Tuesday:
		case NestedTypes::DayOfWeek::Wednesday:
			toWireType(&input->data.foo, &output->data.foo);
			break;
		case NestedTypes::DayOfWeek::Thursday:
		case NestedTypes::DayOfWeek::Friday:
			toWireType(&input->data.bar, &output->data.bar);
			break;
		}
	}

	inline struct NestedTypes::OuterUnion fromWireType(const struct NestedTypes::OuterUnion_wire* input) {
		struct NestedTypes::OuterUnion retval;
		struct NestedTypes::OuterUnion* output = &retval;
		uint32_t tag;
		memcpy(&tag, &input->tag, sizeof(uint32_t));
		tag = be32toh(tag);
		memcpy(&output->tag, &tag, sizeof(uint32_t));
		switch (output->tag) {
		case NestedTypes::DayOfWeek::Monday:
		case NestedTypes::DayOfWeek::Tuesday:
		case NestedTypes::DayOfWeek::Wednesday:
			output->data.foo = fromWireType(&input->data.foo);
			break;
		case NestedTypes::DayOfWeek::Thursday:
		case NestedTypes::DayOfWeek::Friday:
			output->data.bar = fromWireType(&input->data.bar);
			break;
		}
		return retval;
	}

	template<>
	struct Serialization<struct NestedTypes::OuterUnion> {
		static void toBuffer(struct NestedTypes::OuterUnion const& val, std::vector<char>& buf) {
			buf.resize(sizeof(struct NestedTypes::OuterUnion));
			struct NestedTypes::OuterUnion_wire* output = (struct NestedTypes::OuterUnion_wire*) buf.data();
			const struct NestedTypes::OuterUnion* input = &val;
			toWireType(input, output);
		}

		static struct NestedTypes::OuterUnion fromBuffer(std::vector<char> const& buf) {
			const struct NestedTypes::OuterUnion_wire* input = (const struct NestedTypes::OuterUnion_wire*) buf.data();
			if (buf.size() != sizeof(struct NestedTypes::OuterUnion)) {
				static const std::string error_msg =
					std::string("pirate::Serialization::fromBuffer() for NestedTypes::OuterUnion type did not receive a buffer of size ") +
					std::to_string(sizeof(struct NestedTypes::OuterUnion));
				throw std::length_error(error_msg);
			}
			return fromWireType(input);
		}
	};
}

#endif // _NESTEDTYPES_IDL_CODEGEN_H
