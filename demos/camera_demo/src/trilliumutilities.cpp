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

int trilliumConnectUDPSocket(std::string trilliumIpAddress, int& sockFd)
{
    int rv = -1;
    uint32_t address = INADDR_BROADCAST;
    struct sockaddr_in sock_addr;
    int enable = 1;
    struct timeval tv;

    // Validate the address
    rv = inet_pton(AF_INET, trilliumIpAddress.c_str(), &address);
    if (rv != 1)
    {
        std::cerr << "Invalid address format" << trilliumIpAddress << std::endl;
        return -1;
    }

    // Create the UDP socket
    sockFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockFd < 0)
    {
        std::cerr << "Failed to create datagram socket" << std::endl;
        return -1;
    }

    // Allow multiple sockets in the same process use the Trillium command port
    rv = setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if (rv < 0)
    {
        std::cerr << "Failed to set address reuse option" << std::endl;
        goto err;
    }

    // Bind
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    sock_addr.sin_port = htons(UDP_PORT);

    rv = bind(sockFd, (struct sockaddr *)&sock_addr, sizeof(sock_addr));
    if (rv != 0)
    {
        std::cerr << "Failed to bind to the camera UDP port" << std::endl;
        goto err;
    }

    // Connect
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = address;
    sock_addr.sin_port        = htons(UDP_PORT);
    rv = connect(sockFd, (struct sockaddr *)&sock_addr, sizeof(sock_addr));
    if (rv != 0)
    {
        std::cerr << "Connect failed" << std::endl;
        goto err;
    }

    // Set the receive timeout to 1 second
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    if (setsockopt(sockFd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
        std::cerr << "Failed to set socket receive timeout" << std::endl;
        goto err;
    }

    // Success
    return 0;
err:
    int err = errno;
    close(sockFd);
    sockFd = -1;
    errno = err;
    return -1;
}

int trilliumPktSend(int sockFd, OrionPkt_t& pkt)
{
    ssize_t len = pkt.Length + ORION_PKT_OVERHEAD;
    ssize_t rv = -1;

    rv = sendto(sockFd, (char *)&pkt, len, 0, (struct sockaddr *)NULL, 0);
    if (rv != len)
    {
        std::perror("Failed to send datagram packet\n");
        return -1;
    }

    return 0;
}

int trilliumPktRecv(int sockFd, OrionPkt_t& pkt)
{
    ssize_t rxLen;
    rxLen = recvfrom(sockFd, (char *)&pkt, sizeof(OrionPkt_t), 0, (struct sockaddr *)NULL, NULL);

    if (rxLen < ORION_PKT_OVERHEAD)
    {
        return -1;
    }

    return 0;
}
