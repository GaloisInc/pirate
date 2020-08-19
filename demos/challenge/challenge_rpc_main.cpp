#include <memory>

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include "challenge_rpc.grpc.pb.h"

#include "base64.h"
#include "tweetnacl.h"

using google::protobuf::BytesValue;

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

#define KEY_BYTES crypto_secretbox_KEYBYTES
#define NONCE_BYTES crypto_secretbox_NONCEBYTES
#define ZERO_BYTES crypto_secretbox_ZEROBYTES
#define BOX_ZERO_BYTES crypto_secretbox_BOXZEROBYTES
#define DELTA_BYTES (ZERO_BYTES - BOX_ZERO_BYTES)

#if DELTA_BYTES < 0
#error "crypto_secretbox_ZEROBYTES is assumed to be >= crypto_secretbox_BOXZEROBYTES"
#endif

class MainClient {
    public:
        MainClient(std::shared_ptr<Channel> encrypt1_ch, std::shared_ptr<Channel> encrypt2_ch) :
            stub1_(Encrypt1::NewStub(encrypt1_ch)), stub2_(Encrypt2::NewStub(encrypt2_ch)) { }

        int encrypt1(char *input, size_t len, char *output) {
            ClientContext context;
            BytesValue request;
            BytesValue response;

            request.set_value(input, len);
            Status status = stub1_->Encrypt(&context, request, &response);
            if (status.ok() && (response.value().length() == len)) {
                memcpy(output, response.value().data(), len);
                return 0;
            } else {
                return -1;
            }
        }

        int encrypt2(char *input, size_t len, char *output) {
            ClientContext context;
            BytesValue request;
            BytesValue response;

            request.set_value(input, len);
            Status status = stub2_->Encrypt(&context, request, &response);
            if (status.ok() && (response.value().length() == len)) {
                memcpy(output, response.value().data(), len);
                return 0;
            } else {
                return -1;
            }
        }

    private:
        std::unique_ptr<Encrypt1::Stub> stub1_;
        std::unique_ptr<Encrypt2::Stub> stub2_;
};

static const int input_size = 80;
static const int encryption_size = input_size + ZERO_BYTES;
static const int double_encryption_size = encryption_size + DELTA_BYTES;
static const int base64_size = (((double_encryption_size / 3) + 1) * 4);

int main_encryption(char *buffer1, char *buffer2, char *encoded) {
    MainClient client(
        grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()),
        grpc::CreateChannel("localhost:50052", grpc::InsecureChannelCredentials()));

    size_t mlen;
    char *newline, *read_offset = buffer1 + ZERO_BYTES;
    const size_t read_length = input_size;

    char *success = fgets(read_offset, read_length, stdin);
    if (success == NULL) {
        return -1;
    }
    // strip trailing newline
    newline = strrchr(read_offset, '\n');
    if (newline != NULL) {
        *newline = 0;
    }
    mlen = strnlen(read_offset, read_length) + ZERO_BYTES;

    if (client.encrypt1(buffer1, mlen, buffer2 + DELTA_BYTES) < 0) {
        return -1;
    }
    mlen += DELTA_BYTES;
    if (client.encrypt2(buffer2, mlen, buffer1) < 0) {
        return -1;
    }

    base64_encode(encoded, buffer1 + BOX_ZERO_BYTES, mlen - BOX_ZERO_BYTES);
    printf("%s\n", encoded);
    return 0;
}

int main() {
    char *buffer1 = (char*) calloc(double_encryption_size, 1);
    char *buffer2 = (char*) calloc(double_encryption_size, 1);
    char *encoded = (char*) calloc(base64_size, 1);
    int rv = main_encryption(buffer1, buffer2, encoded);
    free(buffer1);
    free(buffer2);
    free(encoded);
    grpc_shutdown_blocking();
    return rv;
}
