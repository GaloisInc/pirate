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

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <string>
#include <iostream>

#include "orion-sdk/OrionPublicPacket.hpp"
#include "orion-sdk/OrionPublicPacketShim.hpp"
#include "orion-sdk/Constants.hpp"
#include "orion-sdk/OrionComm.hpp"

int trilliumConnectUDPSocket(std::string trilliumUrl, int& sockFd)
{
    std::string host = "";
    int port = 0, rv;
    struct sockaddr_in dest_addr;

    if (trilliumUrl == "") {
        std::cerr << "Missing required argument --trillium url" << std::endl;
        return -1;
    }

    std::size_t found = trilliumUrl.find(':');

    if (found == std::string::npos) {
        host = trilliumUrl;
        port = UDP_IN_PORT;
    } else {
        host = trilliumUrl.substr(0, found);
        port = std::stoi(trilliumUrl.substr(found + 1));
    }

    if ((host == "") || (port == 0)) {
        std::cerr << "unable to parse trillium url " << trilliumUrl << std::endl;
        return -1;
    }

    std::cout << "Connecting to trillium camera at " << host << ":" << port << std::endl;

    sockFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockFd < 0) {
        perror("Error creating socket");
        return -1;
    }

    memset(&dest_addr, 0, sizeof(struct sockaddr_in));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(host.c_str());
    dest_addr.sin_port = htons(port);

    rv = connect(sockFd, (const struct sockaddr*) &dest_addr, sizeof(struct sockaddr_in));
    if (rv < 0) {
        perror("Error connecting socket");
        return -1;
    }

    return 0;
}
