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
#include "trilliumutilities.hpp"

#include "orion-sdk/OrionPublicPacket.hpp"
#include "orion-sdk/OrionPublicPacketShim.hpp"
#include "orion-sdk/Constants.hpp"
#include "orion-sdk/OrionComm.hpp"

TrilliumOrientationOutput::TrilliumOrientationOutput(const Options& options) :
    BaseOrientationOutput(options),
    mTrilliumUrl(options.mTrilliumUrl),
    mSockFd(-1)
{
}

TrilliumOrientationOutput::~TrilliumOrientationOutput()
{
    term();
}

int TrilliumOrientationOutput::init()
{
    return trilliumConnectUDPSocket(mTrilliumUrl, mSockFd);
}

void TrilliumOrientationOutput::term()
{
    if (mSockFd >= 0) {
        close(mSockFd);
    }
}


bool TrilliumOrientationOutput::applyAngularPosition(PanTilt angularPosition)
{
    OrionCmd_t orionCmd;
    OrionPkt_t pktOut;
    ssize_t rv;

    if (mSockFd < 0) {
        return false;
    }

    orionCmd.Target[0] = deg2radf(angularPosition.pan);
    orionCmd.Target[1] = deg2radf(angularPosition.tilt);
    orionCmd.Mode = ORION_MODE_POSITION;
    // TODO test the camera with orionCmd.Stabilized = 1
    orionCmd.Stabilized = 0;
    orionCmd.ImpulseTime = 0;

    encodeOrionCmdPacket(&pktOut, &orionCmd);

    rv = send(mSockFd, (void*) &pktOut, pktOut.Length + ORION_PKT_OVERHEAD, 0);
    if (rv < 0) {
        perror("Trillium send command error");
        return false;
    }
    return true;
}
