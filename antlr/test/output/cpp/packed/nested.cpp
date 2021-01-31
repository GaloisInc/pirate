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

	enum class DayOfWeek : uint32_t {
		Monday,
		Tuesday,
		Wednesday,
		Thursday,
		Friday
	};

	struct OuterStruct {
		struct Foo foo;
		struct Bar bar[2][3][4];
		DayOfWeek day __attribute__((aligned(4)));
		DayOfWeek days[30] __attribute__((aligned(4)));
	};

	struct OuterUnion {
		DayOfWeek tag __attribute__((aligned(4)));
		union {
			DayOfWeek day __attribute__((aligned(4)));
			DayOfWeek days[30] __attribute__((aligned(4)));
			struct Foo foo;
			struct Bar bar[2][3][4];
		} data;
	};

	struct ScopedOuterUnion {
		DayOfWeek tag __attribute__((aligned(4)));
		union {
			DayOfWeek day __attribute__((aligned(4)));
			DayOfWeek days[30] __attribute__((aligned(4)));
			struct Foo foo;
			struct Bar bar[2][3][4];
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
		struct Bar_wire bar[2][3][4];
		unsigned char day[4];
		unsigned char days[30][4];
	} __attribute__((packed)) ;

	struct OuterUnion_wire {
		unsigned char tag[4];
		union {
			unsigned char day[4];
			unsigned char days[30][4];
			struct Foo_wire foo;
			struct Bar_wire bar[2][3][4];
		} data;
	} __attribute__((packed)) ;

	struct ScopedOuterUnion_wire {
		unsigned char tag[4];
		union {
			unsigned char day[4];
			unsigned char days[30][4];
			struct Foo_wire foo;
			struct Bar_wire bar[2][3][4];
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
		uint32_t field_day;
		uint32_t field_days;
		for (size_t days_0 = 0; days_0 < 30; days_0++) {
			const NestedTypes::DayOfWeek* inptr = &input->days[days_0];
			unsigned char* outptr = &output->days[days_0][0];
			memcpy(&field_days, inptr, sizeof(uint32_t));
			field_days = htobe32(field_days);
			memcpy(outptr, &field_days, sizeof(uint32_t));
		}
		memcpy(&field_day, &input->day, sizeof(uint32_t));
		field_day = htobe32(field_day);
		memcpy(&output->day, &field_day, sizeof(uint32_t));
		toWireType(&input->foo, &output->foo);
		for (size_t bar_0 = 0; bar_0 < 2; bar_0++) {
			for (size_t bar_1 = 0; bar_1 < 3; bar_1++) {
				for (size_t bar_2 = 0; bar_2 < 4; bar_2++) {
					const struct NestedTypes::Bar* inptr = &input->bar[bar_0][bar_1][bar_2];
					struct NestedTypes::Bar_wire* outptr = &output->bar[bar_0][bar_1][bar_2];
					toWireType(inptr, outptr);
				}
			}
		}
	}

	inline struct NestedTypes::OuterStruct fromWireType(const struct NestedTypes::OuterStruct_wire* input) {
		struct NestedTypes::OuterStruct retval;
		struct NestedTypes::OuterStruct* output = &retval;
		uint32_t field_day;
		uint32_t field_days;
		for (size_t days_0 = 0; days_0 < 30; days_0++) {
			const unsigned char* inptr = &input->days[days_0][0];
			NestedTypes::DayOfWeek* outptr = &output->days[days_0];
			memcpy(&field_days, inptr, sizeof(uint32_t));
			field_days = be32toh(field_days);
			memcpy(outptr, &field_days, sizeof(uint32_t));
		}
		memcpy(&field_day, &input->day, sizeof(uint32_t));
		field_day = be32toh(field_day);
		memcpy(&output->day, &field_day, sizeof(uint32_t));
		output->foo = fromWireType(&input->foo);
		for (size_t bar_0 = 0; bar_0 < 2; bar_0++) {
			for (size_t bar_1 = 0; bar_1 < 3; bar_1++) {
				for (size_t bar_2 = 0; bar_2 < 4; bar_2++) {
					const struct NestedTypes::Bar_wire* inptr = &input->bar[bar_0][bar_1][bar_2];
					struct NestedTypes::Bar* outptr = &output->bar[bar_0][bar_1][bar_2];
					*outptr = fromWireType(inptr);
				}
			}
		}
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
		uint32_t data_day;
		uint32_t data_days;
		memset(output, 0, sizeof(*output));
		memcpy(&tag, &input->tag, sizeof(uint32_t));
		tag = htobe32(tag);
		memcpy(&output->tag, &tag, sizeof(uint32_t));
		switch (input->tag) {
		case NestedTypes::DayOfWeek::Monday:
			memcpy(&data_day, &input->data.day, sizeof(uint32_t));
			data_day = htobe32(data_day);
			memcpy(&output->data.day, &data_day, sizeof(uint32_t));
			break;
		case NestedTypes::DayOfWeek::Tuesday:
			for (size_t days_0 = 0; days_0 < 30; days_0++) {
				const NestedTypes::DayOfWeek* inptr = &input->data.days[days_0];
				unsigned char* outptr = &output->data.days[days_0][0];
				memcpy(&data_days, inptr, sizeof(uint32_t));
				data_days = htobe32(data_days);
				memcpy(outptr, &data_days, sizeof(uint32_t));
			}
			break;
		case NestedTypes::DayOfWeek::Wednesday:
			toWireType(&input->data.foo, &output->data.foo);
			break;
		case NestedTypes::DayOfWeek::Thursday:
		case NestedTypes::DayOfWeek::Friday:
			for (size_t bar_0 = 0; bar_0 < 2; bar_0++) {
				for (size_t bar_1 = 0; bar_1 < 3; bar_1++) {
					for (size_t bar_2 = 0; bar_2 < 4; bar_2++) {
						const struct NestedTypes::Bar* inptr = &input->data.bar[bar_0][bar_1][bar_2];
						struct NestedTypes::Bar_wire* outptr = &output->data.bar[bar_0][bar_1][bar_2];
						toWireType(inptr, outptr);
					}
				}
			}
			break;
		}
	}

	inline struct NestedTypes::OuterUnion fromWireType(const struct NestedTypes::OuterUnion_wire* input) {
		struct NestedTypes::OuterUnion retval;
		struct NestedTypes::OuterUnion* output = &retval;
		uint32_t tag;
		uint32_t data_day;
		uint32_t data_days;
		memcpy(&tag, &input->tag, sizeof(uint32_t));
		tag = be32toh(tag);
		memcpy(&output->tag, &tag, sizeof(uint32_t));
		switch (output->tag) {
		case NestedTypes::DayOfWeek::Monday:
			memcpy(&data_day, &input->data.day, sizeof(uint32_t));
			data_day = be32toh(data_day);
			memcpy(&output->data.day, &data_day, sizeof(uint32_t));
			break;
		case NestedTypes::DayOfWeek::Tuesday:
			for (size_t days_0 = 0; days_0 < 30; days_0++) {
				const unsigned char* inptr = &input->data.days[days_0][0];
				NestedTypes::DayOfWeek* outptr = &output->data.days[days_0];
				memcpy(&data_days, inptr, sizeof(uint32_t));
				data_days = be32toh(data_days);
				memcpy(outptr, &data_days, sizeof(uint32_t));
			}
			break;
		case NestedTypes::DayOfWeek::Wednesday:
			output->data.foo = fromWireType(&input->data.foo);
			break;
		case NestedTypes::DayOfWeek::Thursday:
		case NestedTypes::DayOfWeek::Friday:
			for (size_t bar_0 = 0; bar_0 < 2; bar_0++) {
				for (size_t bar_1 = 0; bar_1 < 3; bar_1++) {
					for (size_t bar_2 = 0; bar_2 < 4; bar_2++) {
						const struct NestedTypes::Bar_wire* inptr = &input->data.bar[bar_0][bar_1][bar_2];
						struct NestedTypes::Bar* outptr = &output->data.bar[bar_0][bar_1][bar_2];
						*outptr = fromWireType(inptr);
					}
				}
			}
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

	inline void toWireType(const struct NestedTypes::ScopedOuterUnion* input, struct NestedTypes::ScopedOuterUnion_wire* output) {
		uint32_t tag;
		uint32_t data_day;
		uint32_t data_days;
		memset(output, 0, sizeof(*output));
		memcpy(&tag, &input->tag, sizeof(uint32_t));
		tag = htobe32(tag);
		memcpy(&output->tag, &tag, sizeof(uint32_t));
		switch (input->tag) {
		case NestedTypes::DayOfWeek::Monday:
			memcpy(&data_day, &input->data.day, sizeof(uint32_t));
			data_day = htobe32(data_day);
			memcpy(&output->data.day, &data_day, sizeof(uint32_t));
			break;
		case NestedTypes::DayOfWeek::Tuesday:
			for (size_t days_0 = 0; days_0 < 30; days_0++) {
				const NestedTypes::DayOfWeek* inptr = &input->data.days[days_0];
				unsigned char* outptr = &output->data.days[days_0][0];
				memcpy(&data_days, inptr, sizeof(uint32_t));
				data_days = htobe32(data_days);
				memcpy(outptr, &data_days, sizeof(uint32_t));
			}
			break;
		case NestedTypes::DayOfWeek::Wednesday:
			toWireType(&input->data.foo, &output->data.foo);
			break;
		case NestedTypes::DayOfWeek::Thursday:
		case NestedTypes::DayOfWeek::Friday:
			for (size_t bar_0 = 0; bar_0 < 2; bar_0++) {
				for (size_t bar_1 = 0; bar_1 < 3; bar_1++) {
					for (size_t bar_2 = 0; bar_2 < 4; bar_2++) {
						const struct NestedTypes::Bar* inptr = &input->data.bar[bar_0][bar_1][bar_2];
						struct NestedTypes::Bar_wire* outptr = &output->data.bar[bar_0][bar_1][bar_2];
						toWireType(inptr, outptr);
					}
				}
			}
			break;
		}
	}

	inline struct NestedTypes::ScopedOuterUnion fromWireType(const struct NestedTypes::ScopedOuterUnion_wire* input) {
		struct NestedTypes::ScopedOuterUnion retval;
		struct NestedTypes::ScopedOuterUnion* output = &retval;
		uint32_t tag;
		uint32_t data_day;
		uint32_t data_days;
		memcpy(&tag, &input->tag, sizeof(uint32_t));
		tag = be32toh(tag);
		memcpy(&output->tag, &tag, sizeof(uint32_t));
		switch (output->tag) {
		case NestedTypes::DayOfWeek::Monday:
			memcpy(&data_day, &input->data.day, sizeof(uint32_t));
			data_day = be32toh(data_day);
			memcpy(&output->data.day, &data_day, sizeof(uint32_t));
			break;
		case NestedTypes::DayOfWeek::Tuesday:
			for (size_t days_0 = 0; days_0 < 30; days_0++) {
				const unsigned char* inptr = &input->data.days[days_0][0];
				NestedTypes::DayOfWeek* outptr = &output->data.days[days_0];
				memcpy(&data_days, inptr, sizeof(uint32_t));
				data_days = be32toh(data_days);
				memcpy(outptr, &data_days, sizeof(uint32_t));
			}
			break;
		case NestedTypes::DayOfWeek::Wednesday:
			output->data.foo = fromWireType(&input->data.foo);
			break;
		case NestedTypes::DayOfWeek::Thursday:
		case NestedTypes::DayOfWeek::Friday:
			for (size_t bar_0 = 0; bar_0 < 2; bar_0++) {
				for (size_t bar_1 = 0; bar_1 < 3; bar_1++) {
					for (size_t bar_2 = 0; bar_2 < 4; bar_2++) {
						const struct NestedTypes::Bar_wire* inptr = &input->data.bar[bar_0][bar_1][bar_2];
						struct NestedTypes::Bar* outptr = &output->data.bar[bar_0][bar_1][bar_2];
						*outptr = fromWireType(inptr);
					}
				}
			}
			break;
		}
		return retval;
	}

	template<>
	struct Serialization<struct NestedTypes::ScopedOuterUnion> {
		static void toBuffer(struct NestedTypes::ScopedOuterUnion const& val, std::vector<char>& buf) {
			buf.resize(sizeof(struct NestedTypes::ScopedOuterUnion));
			struct NestedTypes::ScopedOuterUnion_wire* output = (struct NestedTypes::ScopedOuterUnion_wire*) buf.data();
			const struct NestedTypes::ScopedOuterUnion* input = &val;
			toWireType(input, output);
		}

		static struct NestedTypes::ScopedOuterUnion fromBuffer(std::vector<char> const& buf) {
			const struct NestedTypes::ScopedOuterUnion_wire* input = (const struct NestedTypes::ScopedOuterUnion_wire*) buf.data();
			if (buf.size() != sizeof(struct NestedTypes::ScopedOuterUnion)) {
				static const std::string error_msg =
					std::string("pirate::Serialization::fromBuffer() for NestedTypes::ScopedOuterUnion type did not receive a buffer of size ") +
					std::to_string(sizeof(struct NestedTypes::ScopedOuterUnion));
				throw std::length_error(error_msg);
			}
			return fromWireType(input);
		}
	};
}

#endif // _NESTEDTYPES_IDL_CODEGEN_H
