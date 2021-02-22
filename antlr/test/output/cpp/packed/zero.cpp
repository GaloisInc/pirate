#ifndef _ZERO_IDL_CODEGEN_H
#define _ZERO_IDL_CODEGEN_H

#include <cassert>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

#include <endian.h>

namespace Zero {

	struct Zero {
	};

	struct Zero_wire {
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

	inline void toWireType(const struct Zero::Zero* input, struct Zero::Zero_wire* output) {
		(void) input;
		(void) output;
	}

	inline struct Zero::Zero fromWireType(const struct Zero::Zero_wire* input) {
		struct Zero::Zero retval;
		(void) input;
		return retval;
	}

	template<>
	struct Serialization<struct Zero::Zero> {
		static void toBuffer(struct Zero::Zero const& val, std::vector<char>& buf) {
			buf.resize(sizeof(struct Zero::Zero_wire));
			struct Zero::Zero_wire* output = (struct Zero::Zero_wire*) buf.data();
			const struct Zero::Zero* input = &val;
			toWireType(input, output);
		}

		static struct Zero::Zero fromBuffer(std::vector<char> const& buf) {
			const struct Zero::Zero_wire* input = (const struct Zero::Zero_wire*) buf.data();
			if (buf.size() != sizeof(struct Zero::Zero_wire)) {
				static const std::string error_msg =
					std::string("pirate::Serialization::fromBuffer() for Zero::Zero type did not receive a buffer of size ") +
					std::to_string(sizeof(struct Zero::Zero_wire));
				throw std::length_error(error_msg);
			}
			return fromWireType(input);
		}
	};
}

#endif // _ZERO_IDL_CODEGEN_H
