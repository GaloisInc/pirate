/*
 * This work was authored by Two Six Labs, LLC and is sponsored by a subcontract
 * agreement with Galois, Inc.  This material is based upon work supported by
 * the Defense Advanced Research Projects Agency (DARPA) under Contract No.
 * HR0011-19-C-0103.
 *
 * The Government has unlimited rights to use, modify, reproduce, release,
 * perform, display, or disclose computer software or computer software
 * documentation marked with this legend. Any reproduction of technical data,
 * computer software, or portions thereof marked with this legend must also
 * reproduce this marking.
 *
 * Copyright 2020 Two Six Labs, LLC.  All rights reserved.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <activemq/library/ActiveMQCPP.h>
#include <activemq/core/ActiveMQConnectionFactory.h>
#include <activemq/util/Config.h>

#include "tweetnacl.h"
#include "base64.h"

using namespace activemq::core;
using namespace cms;

const std::string brokerURI = "tcp://localhost:61616?wireFormat=openwire";

#define KEY_BYTES crypto_secretbox_KEYBYTES
#define NONCE_BYTES crypto_secretbox_NONCEBYTES
#define ZERO_BYTES crypto_secretbox_ZEROBYTES
#define BOX_ZERO_BYTES crypto_secretbox_BOXZEROBYTES
#define DELTA_BYTES (ZERO_BYTES - BOX_ZERO_BYTES)

#if DELTA_BYTES < 0
#error "crypto_secretbox_ZEROBYTES is assumed to be >= crypto_secretbox_BOXZEROBYTES"
#endif

static const unsigned char key2[KEY_BYTES] = "secret key 2";
static unsigned char nonce2[NONCE_BYTES] = {0};

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

void encrypt2(char *input, size_t len, char *output) {
    crypto_secretbox((unsigned char*) output, (unsigned char*) input, len, nonce2, key2);
    increment_nonce(nonce2, NONCE_BYTES);
}

static const int input_size = 80;
static const int encryption_size = input_size + ZERO_BYTES;
static const int double_encryption_size = encryption_size + DELTA_BYTES;
static const int base64_size = (((double_encryption_size / 3) + 1) * 4);

int main_encryption(char *buffer1, char *buffer2) {
    ssize_t mlen;

    activemq::library::ActiveMQCPP::initializeLibrary();
    ConnectionFactory* connectionFactory = ConnectionFactory::createCMSConnectionFactory(brokerURI);
    Connection* connection = connectionFactory->createConnection();
    connection->start();
    Session* session = connection->createSession(Session::AUTO_ACKNOWLEDGE);
    // one consumer and one producer
    Destination* enclave2_dest = session->createQueue("enclave2");
    // one consumer and two producers
    Destination* main_dest = session->createQueue("main");
    MessageConsumer* enclave2_consumer = session->createConsumer(enclave2_dest);
    MessageProducer* main_producer = session->createProducer(main_dest);

    main_producer->setDeliveryMode(DeliveryMode::NON_PERSISTENT);

    BytesMessage* message = dynamic_cast<BytesMessage*>(enclave2_consumer->receive());
    mlen = message->getBodyLength();
    message->readBytes((unsigned char *) buffer1, mlen);
    encrypt2(buffer1, mlen, buffer2);
    message->clearBody();
    message->writeBytes((unsigned char *) buffer2, 0, mlen);
    main_producer->send(message);
    delete(message);

    connection->close();

    delete main_producer;
    delete enclave2_consumer;
    delete main_dest;
    delete enclave2_dest;
    delete session;
    delete connection;
    delete connectionFactory;

    activemq::library::ActiveMQCPP::shutdownLibrary();

    return 0;
}

int main() {
    char *buffer1 = (char*) calloc(double_encryption_size, 1);
    char *buffer2 = (char*) calloc(double_encryption_size, 1);
    int rv = main_encryption(buffer1, buffer2);
    free(buffer1);
    free(buffer2);
    return rv;
}
