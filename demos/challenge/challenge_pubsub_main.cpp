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

const std::string brokerURI = "tcp://localhost:61616?wireFormat=openwire&connectionTimeout=1000";

#define KEY_BYTES crypto_secretbox_KEYBYTES
#define NONCE_BYTES crypto_secretbox_NONCEBYTES
#define ZERO_BYTES crypto_secretbox_ZEROBYTES
#define BOX_ZERO_BYTES crypto_secretbox_BOXZEROBYTES
#define DELTA_BYTES (ZERO_BYTES - BOX_ZERO_BYTES)

#if DELTA_BYTES < 0
#error "crypto_secretbox_ZEROBYTES is assumed to be >= crypto_secretbox_BOXZEROBYTES"
#endif

static const int input_size = 80;
static const int encryption_size = input_size + ZERO_BYTES;
static const int double_encryption_size = encryption_size + DELTA_BYTES;
static const int base64_size = (((double_encryption_size / 3) + 1) * 4);

int main_encryption(char *buffer1, char *buffer2, char *encoded) {
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
    
    activemq::library::ActiveMQCPP::initializeLibrary();
    ConnectionFactory* connectionFactory = ConnectionFactory::createCMSConnectionFactory(brokerURI);
    Connection* connection = connectionFactory->createConnection();
    connection->start();
    Session* session = connection->createSession(Session::AUTO_ACKNOWLEDGE);
    // one consumer and one producer
    Destination* enclave1_dest = session->createQueue("enclave1");
    // one consumer and one producer
    Destination* enclave2_dest = session->createQueue("enclave2");
    // one consumer and two producers
    Destination* main_dest = session->createQueue("main");
    MessageProducer* enclave1_producer = session->createProducer(enclave1_dest);
    MessageProducer* enclave2_producer = session->createProducer(enclave2_dest);
    MessageConsumer* main_consumer = session->createConsumer(main_dest);

    enclave1_producer->setDeliveryMode(DeliveryMode::NON_PERSISTENT);
    enclave2_producer->setDeliveryMode(DeliveryMode::NON_PERSISTENT);

    BytesMessage* message = session->createBytesMessage((const unsigned char*) buffer1, mlen);
    enclave1_producer->send(message);
    delete(message);
    message = dynamic_cast<BytesMessage*>(main_consumer->receive());
    message->readBytes((unsigned char *) buffer2 + DELTA_BYTES, mlen);
    message->clearBody();
    mlen += DELTA_BYTES;
    message->writeBytes((unsigned char *) buffer2, 0, mlen);
    enclave2_producer->send(message);
    delete(message);
    message = dynamic_cast<BytesMessage*>(main_consumer->receive());
    message->readBytes((unsigned char *) buffer1, mlen);
    delete(message);

    base64_encode(encoded, buffer1 + BOX_ZERO_BYTES, mlen - BOX_ZERO_BYTES);
    printf("%s\n", encoded);

    connection->close();

    delete main_consumer;
    delete enclave1_producer;
    delete enclave2_producer;
    delete main_dest;
    delete enclave1_dest;
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
    char *encoded = (char*) calloc(base64_size, 1);
    int rv = main_encryption(buffer1, buffer2, encoded);
    free(buffer1);
    free(buffer2);
    free(encoded);
    return rv;
}
