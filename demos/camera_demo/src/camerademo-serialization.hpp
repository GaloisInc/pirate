#ifndef _CAMERADEMO_IDL_CODEGEN_H
#define _CAMERADEMO_IDL_CODEGEN_H

#include <cassert>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

#include <endian.h>

namespace CameraDemo {

	enum class OrientationOutputReqType : uint32_t {
		OutputGet,
		OutputSet,
		OutputUpdate
	};

	struct OrientationOutputRequest {
		uint16_t clientId __attribute__((aligned(2)));
		uint16_t messageId __attribute__((aligned(2)));
		OrientationOutputReqType reqType __attribute__((aligned(4)));
		float angularPosition __attribute__((aligned(4)));
	};

	struct OrientationOutputResponse {
		float angularPosition __attribute__((aligned(4)));
	};

	struct OrientationOutputRequest_wire {
		unsigned char clientId[2] __attribute__((aligned(2)));
		unsigned char messageId[2] __attribute__((aligned(2)));
		unsigned char reqType[4] __attribute__((aligned(4)));
		unsigned char angularPosition[4] __attribute__((aligned(4)));
	};

	struct OrientationOutputResponse_wire {
		unsigned char angularPosition[4] __attribute__((aligned(4)));
	};

	static_assert(sizeof(struct OrientationOutputRequest) == sizeof(struct OrientationOutputRequest_wire), "size of struct OrientationOutputRequest not equal to wire protocol struct");
	static_assert(sizeof(struct OrientationOutputResponse) == sizeof(struct OrientationOutputResponse_wire), "size of struct OrientationOutputResponse not equal to wire protocol struct");
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
	struct Serialization<struct CameraDemo::OrientationOutputRequest> {
		static void toBuffer(struct CameraDemo::OrientationOutputRequest const& val, std::vector<char>& buf) {
			buf.resize(sizeof(struct CameraDemo::OrientationOutputRequest));
			struct CameraDemo::OrientationOutputRequest_wire* output = (struct CameraDemo::OrientationOutputRequest_wire*) buf.data();
			const struct CameraDemo::OrientationOutputRequest* input = &val;
			uint16_t field_clientId;
			uint16_t field_messageId;
			uint32_t field_reqType;
			uint32_t field_angularPosition;
			memcpy(&field_clientId, &input->clientId, sizeof(uint16_t));
			memcpy(&field_messageId, &input->messageId, sizeof(uint16_t));
			memcpy(&field_reqType, &input->reqType, sizeof(uint32_t));
			memcpy(&field_angularPosition, &input->angularPosition, sizeof(uint32_t));
			field_clientId = htobe16(field_clientId);
			field_messageId = htobe16(field_messageId);
			field_reqType = htobe32(field_reqType);
			field_angularPosition = htobe32(field_angularPosition);
			memcpy(&output->clientId, &field_clientId, sizeof(uint16_t));
			memcpy(&output->messageId, &field_messageId, sizeof(uint16_t));
			memcpy(&output->reqType, &field_reqType, sizeof(uint32_t));
			memcpy(&output->angularPosition, &field_angularPosition, sizeof(uint32_t));
		}

		static struct CameraDemo::OrientationOutputRequest fromBuffer(std::vector<char> const& buf) {
			struct CameraDemo::OrientationOutputRequest retval;
			const struct CameraDemo::OrientationOutputRequest_wire* input = (const struct CameraDemo::OrientationOutputRequest_wire*) buf.data();
			struct CameraDemo::OrientationOutputRequest* output = &retval;
			if (buf.size() != sizeof(struct CameraDemo::OrientationOutputRequest)) {
				static const std::string error_msg =
					std::string("pirate::Serialization::fromBuffer() for CameraDemo::OrientationOutputRequest type did not receive a buffer of size ") +
					std::to_string(sizeof(struct CameraDemo::OrientationOutputRequest));
				throw std::length_error(error_msg);
			}
			uint16_t field_clientId;
			uint16_t field_messageId;
			uint32_t field_reqType;
			uint32_t field_angularPosition;
			memcpy(&field_clientId, &input->clientId, sizeof(uint16_t));
			memcpy(&field_messageId, &input->messageId, sizeof(uint16_t));
			memcpy(&field_reqType, &input->reqType, sizeof(uint32_t));
			memcpy(&field_angularPosition, &input->angularPosition, sizeof(uint32_t));
			field_clientId = be16toh(field_clientId);
			field_messageId = be16toh(field_messageId);
			field_reqType = be32toh(field_reqType);
			field_angularPosition = be32toh(field_angularPosition);
			memcpy(&output->clientId, &field_clientId, sizeof(uint16_t));
			memcpy(&output->messageId, &field_messageId, sizeof(uint16_t));
			memcpy(&output->reqType, &field_reqType, sizeof(uint32_t));
			memcpy(&output->angularPosition, &field_angularPosition, sizeof(uint32_t));
			return retval;
		}
	};

	template<>
	struct Serialization<struct CameraDemo::OrientationOutputResponse> {
		static void toBuffer(struct CameraDemo::OrientationOutputResponse const& val, std::vector<char>& buf) {
			buf.resize(sizeof(struct CameraDemo::OrientationOutputResponse));
			struct CameraDemo::OrientationOutputResponse_wire* output = (struct CameraDemo::OrientationOutputResponse_wire*) buf.data();
			const struct CameraDemo::OrientationOutputResponse* input = &val;
			uint32_t field_angularPosition;
			memcpy(&field_angularPosition, &input->angularPosition, sizeof(uint32_t));
			field_angularPosition = htobe32(field_angularPosition);
			memcpy(&output->angularPosition, &field_angularPosition, sizeof(uint32_t));
		}

		static struct CameraDemo::OrientationOutputResponse fromBuffer(std::vector<char> const& buf) {
			struct CameraDemo::OrientationOutputResponse retval;
			const struct CameraDemo::OrientationOutputResponse_wire* input = (const struct CameraDemo::OrientationOutputResponse_wire*) buf.data();
			struct CameraDemo::OrientationOutputResponse* output = &retval;
			if (buf.size() != sizeof(struct CameraDemo::OrientationOutputResponse)) {
				static const std::string error_msg =
					std::string("pirate::Serialization::fromBuffer() for CameraDemo::OrientationOutputResponse type did not receive a buffer of size ") +
					std::to_string(sizeof(struct CameraDemo::OrientationOutputResponse));
				throw std::length_error(error_msg);
			}
			uint32_t field_angularPosition;
			memcpy(&field_angularPosition, &input->angularPosition, sizeof(uint32_t));
			field_angularPosition = be32toh(field_angularPosition);
			memcpy(&output->angularPosition, &field_angularPosition, sizeof(uint32_t));
			return retval;
		}
	};
}

#endif // _CAMERADEMO_IDL_CODEGEN_H
