#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>

#include "challenge_rpc.grpc.pb.h"

#include "tweetnacl.h"
#include "base64.h"

using google::protobuf::BytesValue;

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

#define KEY_BYTES crypto_secretbox_KEYBYTES
#define NONCE_BYTES crypto_secretbox_NONCEBYTES
#define ZERO_BYTES crypto_secretbox_ZEROBYTES
#define BOX_ZERO_BYTES crypto_secretbox_BOXZEROBYTES
#define DELTA_BYTES (ZERO_BYTES - BOX_ZERO_BYTES)

#if DELTA_BYTES < 0
#error "crypto_secretbox_ZEROBYTES is assumed to be >= crypto_secretbox_BOXZEROBYTES"
#endif

static const unsigned char key1[KEY_BYTES] = "secret key 1";
static unsigned char nonce1[NONCE_BYTES] = {0};

static void increment_nonce(unsigned char *n, const size_t nlen) {
    size_t i = 0U;
    uint_fast16_t c = 1U;
    for (; i < nlen; i++) {
        c += (uint_fast16_t) n[i];
        n[i] = (unsigned char) c;
        c >>= 8;
    }
}

/**
 * Assume that encrypt1() and encrypt2() use different encryption
 * algorithms that are provided by different encryption libraries.
 *
 * For simplicity they both use crypto_secretbox_xsalsa20poly1305
 * primitive by the TweetNaCl library https://tweetnacl.cr.yp.to
 **/

void encrypt1(char *input, size_t len, char *output) {
    crypto_secretbox((unsigned char*) output, (unsigned char*) input, len, nonce1, key1);
    increment_nonce(nonce1, NONCE_BYTES);
}

class EncryptServiceImpl final : public Encrypt1::Service {
    public:
        Status Encrypt(ServerContext* context, const BytesValue* request, BytesValue* response) override {
            (void) context;
            const std::string input = request->value();
            std::string output;
            output.resize(input.length());
            encrypt1((char*) input.data(), input.length(), (char*) output.data());
            response->set_value(output);
            return Status::OK;
        }
};


void RunServer() {
    std::string server_address("0.0.0.0:50051");
    EncryptServiceImpl service;
    
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}

int main() {
  RunServer();
  return 0;
}
