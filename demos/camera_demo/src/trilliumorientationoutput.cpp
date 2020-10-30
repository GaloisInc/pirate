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

#include "trilliumorientationoutput.hpp"

#include "orion-sdk/OrionPublicPacket.hpp"
#include "orion-sdk/OrionPublicPacketShim.hpp"
#include "orion-sdk/Constants.hpp"
#include "orion-sdk/OrionComm.hpp"

TrilliumOrientationOutput::TrilliumOrientationOutput(const Options& options) :
    BaseOrientationOutput(options),
    mTrilliumUrl(options.mTrilliumUrl),
    sockFd(-1)
{
}

TrilliumOrientationOutput::~TrilliumOrientationOutput()
{
    term();
}

int TrilliumOrientationOutput::init()
{
    std::string host = "";
    int port = 0, rv;
    struct sockaddr_in dest_addr;

    if (mTrilliumUrl == "") {
        std::cerr << "Missing required argument --trillium url" << std::endl;
        return -1;
    }

    std::size_t found = mTrilliumUrl.find(':');

    if (found == std::string::npos) {
        host = mTrilliumUrl;
        port = UDP_IN_PORT;
    } else {
        host = mTrilliumUrl.substr(0, found);
        port = stoi(mTrilliumUrl.substr(found + 1));
    }

    if ((host == "") || (port == 0)) {
        std::cerr << "unable to parse trillium url " << mTrilliumUrl << std::endl;
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

void TrilliumOrientationOutput::term()
{
    if (sockFd >= 0) {
        close(sockFd);
    }
}


bool TrilliumOrientationOutput::applyAngularPosition(PanTilt angularPosition)
{
    OrionCmd_t orionCmd;
    OrionPkt_t pktOut;
    ssize_t rv;

    if (sockFd < 0) {
        return false;
    }

    orionCmd.Target[0] = deg2radf(angularPosition.pan);
    orionCmd.Target[1] = deg2radf(angularPosition.tilt);
    orionCmd.Mode = ORION_MODE_POSITION;
    // TODO test the camera with orionCmd.Stabilized = 1
    orionCmd.Stabilized = 0;
    orionCmd.ImpulseTime = 0;

    encodeOrionCmdPacket(&pktOut, &orionCmd);

    rv = send(sockFd, (void*) &pktOut, pktOut.Length + ORION_PKT_OVERHEAD, 0);
    if (rv < 0) {
        perror("Trillium send command error");
        return false;
    }
    return true;
}
