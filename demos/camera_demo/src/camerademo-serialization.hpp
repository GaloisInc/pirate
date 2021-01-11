#ifndef _CAMERADEMO_IDL_CODEGEN_H
#define _CAMERADEMO_IDL_CODEGEN_H

#include <cassert>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

#include <endian.h>

namespace CameraDemo {

	enum class CameraControlOutputReqType : uint32_t {
		OutputGet,
		OutputSet,
		OutputUpdate
	};

	struct CameraControlOutputRequest {
		uint16_t messageId __attribute__((aligned(2)));
		CameraControlOutputReqType reqType __attribute__((aligned(4)));
		float angularPositionPan __attribute__((aligned(4)));
		float angularPositionTilt __attribute__((aligned(4)));
	};

	struct CameraControlOutputResponse {
		float angularPositionPan __attribute__((aligned(4)));
		float angularPositionTilt __attribute__((aligned(4)));
	};

	struct CameraControlOutputRequest_wire {
		unsigned char messageId[2] __attribute__((aligned(2)));
		unsigned char reqType[4] __attribute__((aligned(4)));
		unsigned char angularPositionPan[4] __attribute__((aligned(4)));
		unsigned char angularPositionTilt[4] __attribute__((aligned(4)));
	};

	struct CameraControlOutputResponse_wire {
		unsigned char angularPositionPan[4] __attribute__((aligned(4)));
		unsigned char angularPositionTilt[4] __attribute__((aligned(4)));
	};

	static_assert(sizeof(struct CameraControlOutputRequest) == sizeof(struct CameraControlOutputRequest_wire), "size of struct CameraControlOutputRequest not equal to wire protocol struct");
	static_assert(sizeof(struct CameraControlOutputResponse) == sizeof(struct CameraControlOutputResponse_wire), "size of struct CameraControlOutputResponse not equal to wire protocol struct");
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
	struct Serialization<struct CameraDemo::CameraControlOutputRequest> {
		static void toBuffer(struct CameraDemo::CameraControlOutputRequest const& val, std::vector<char>& buf) {
			buf.resize(sizeof(struct CameraDemo::CameraControlOutputRequest));
			struct CameraDemo::CameraControlOutputRequest_wire* output = (struct CameraDemo::CameraControlOutputRequest_wire*) buf.data();
			const struct CameraDemo::CameraControlOutputRequest* input = &val;
			uint16_t field_messageId;
			uint32_t field_reqType;
			uint32_t field_angularPositionPan;
			uint32_t field_angularPositionTilt;
			memcpy(&field_messageId, &input->messageId, sizeof(uint16_t));
			memcpy(&field_reqType, &input->reqType, sizeof(uint32_t));
			memcpy(&field_angularPositionPan, &input->angularPositionPan, sizeof(uint32_t));
			memcpy(&field_angularPositionTilt, &input->angularPositionTilt, sizeof(uint32_t));
			field_messageId = htobe16(field_messageId);
			field_reqType = htobe32(field_reqType);
			field_angularPositionPan = htobe32(field_angularPositionPan);
			field_angularPositionTilt = htobe32(field_angularPositionTilt);
			memcpy(&output->messageId, &field_messageId, sizeof(uint16_t));
			memcpy(&output->reqType, &field_reqType, sizeof(uint32_t));
			memcpy(&output->angularPositionPan, &field_angularPositionPan, sizeof(uint32_t));
			memcpy(&output->angularPositionTilt, &field_angularPositionTilt, sizeof(uint32_t));
		}

		static struct CameraDemo::CameraControlOutputRequest fromBuffer(std::vector<char> const& buf) {
			struct CameraDemo::CameraControlOutputRequest retval;
			const struct CameraDemo::CameraControlOutputRequest_wire* input = (const struct CameraDemo::CameraControlOutputRequest_wire*) buf.data();
			struct CameraDemo::CameraControlOutputRequest* output = &retval;
			if (buf.size() != sizeof(struct CameraDemo::CameraControlOutputRequest)) {
				static const std::string error_msg =
					std::string("pirate::Serialization::fromBuffer() for CameraDemo::CameraControlOutputRequest type did not receive a buffer of size ") +
					std::to_string(sizeof(struct CameraDemo::CameraControlOutputRequest));
				throw std::length_error(error_msg);
			}
			uint16_t field_messageId;
			uint32_t field_reqType;
			uint32_t field_angularPositionPan;
			uint32_t field_angularPositionTilt;
			memcpy(&field_messageId, &input->messageId, sizeof(uint16_t));
			memcpy(&field_reqType, &input->reqType, sizeof(uint32_t));
			memcpy(&field_angularPositionPan, &input->angularPositionPan, sizeof(uint32_t));
			memcpy(&field_angularPositionTilt, &input->angularPositionTilt, sizeof(uint32_t));
			field_messageId = be16toh(field_messageId);
			field_reqType = be32toh(field_reqType);
			field_angularPositionPan = be32toh(field_angularPositionPan);
			field_angularPositionTilt = be32toh(field_angularPositionTilt);
			memcpy(&output->messageId, &field_messageId, sizeof(uint16_t));
			memcpy(&output->reqType, &field_reqType, sizeof(uint32_t));
			memcpy(&output->angularPositionPan, &field_angularPositionPan, sizeof(uint32_t));
			memcpy(&output->angularPositionTilt, &field_angularPositionTilt, sizeof(uint32_t));
			return retval;
		}
	};

	template<>
	struct Serialization<struct CameraDemo::CameraControlOutputResponse> {
		static void toBuffer(struct CameraDemo::CameraControlOutputResponse const& val, std::vector<char>& buf) {
			buf.resize(sizeof(struct CameraDemo::CameraControlOutputResponse));
			struct CameraDemo::CameraControlOutputResponse_wire* output = (struct CameraDemo::CameraControlOutputResponse_wire*) buf.data();
			const struct CameraDemo::CameraControlOutputResponse* input = &val;
			uint32_t field_angularPositionPan;
			uint32_t field_angularPositionTilt;
			memcpy(&field_angularPositionPan, &input->angularPositionPan, sizeof(uint32_t));
			memcpy(&field_angularPositionTilt, &input->angularPositionTilt, sizeof(uint32_t));
			field_angularPositionPan = htobe32(field_angularPositionPan);
			field_angularPositionTilt = htobe32(field_angularPositionTilt);
			memcpy(&output->angularPositionPan, &field_angularPositionPan, sizeof(uint32_t));
			memcpy(&output->angularPositionTilt, &field_angularPositionTilt, sizeof(uint32_t));
		}

		static struct CameraDemo::CameraControlOutputResponse fromBuffer(std::vector<char> const& buf) {
			struct CameraDemo::CameraControlOutputResponse retval;
			const struct CameraDemo::CameraControlOutputResponse_wire* input = (const struct CameraDemo::CameraControlOutputResponse_wire*) buf.data();
			struct CameraDemo::CameraControlOutputResponse* output = &retval;
			if (buf.size() != sizeof(struct CameraDemo::CameraControlOutputResponse)) {
				static const std::string error_msg =
					std::string("pirate::Serialization::fromBuffer() for CameraDemo::CameraControlOutputResponse type did not receive a buffer of size ") +
					std::to_string(sizeof(struct CameraDemo::CameraControlOutputResponse));
				throw std::length_error(error_msg);
			}
			uint32_t field_angularPositionPan;
			uint32_t field_angularPositionTilt;
			memcpy(&field_angularPositionPan, &input->angularPositionPan, sizeof(uint32_t));
			memcpy(&field_angularPositionTilt, &input->angularPositionTilt, sizeof(uint32_t));
			field_angularPositionPan = be32toh(field_angularPositionPan);
			field_angularPositionTilt = be32toh(field_angularPositionTilt);
			memcpy(&output->angularPositionPan, &field_angularPositionPan, sizeof(uint32_t));
			memcpy(&output->angularPositionTilt, &field_angularPositionTilt, sizeof(uint32_t));
			return retval;
		}
	};
}

#endif // _CAMERADEMO_IDL_CODEGEN_H
