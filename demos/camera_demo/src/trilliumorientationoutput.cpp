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

#include <cstring>

#include "trilliumorientationoutput.hpp"
#include "trilliumutilities.hpp"

#include "orion-sdk/OrionPublicPacket.hpp"
#include "orion-sdk/OrionPublicPacketShim.hpp"
#include "orion-sdk/Constants.hpp"
#include "orion-sdk/OrionComm.hpp"

TrilliumOrientationOutput::TrilliumOrientationOutput(const Options& options) :
    BaseOrientationOutput(options),
    mTrilliumIpAddress(options.mTrilliumIpAddress),
    mSockFd(-1)
{
}

TrilliumOrientationOutput::~TrilliumOrientationOutput()
{
    term();
}

int TrilliumOrientationOutput::init()
{
    return trilliumConnectUDPSocket(mTrilliumIpAddress, mSockFd);
}

void TrilliumOrientationOutput::term()
{
    if (mSockFd >= 0)
    {
        close(mSockFd);
    }
}


bool TrilliumOrientationOutput::applyAngularPosition(PanTilt angularPosition)
{
    OrionCmd_t cmd;
    OrionPkt_t pkt;

    if (mSockFd < 0)
    {
        return false;
    }

    std::memset(&cmd, 0, sizeof(cmd));
    cmd.Target[0] = deg2radf(angularPosition.pan);
    cmd.Target[1] = deg2radf(angularPosition.tilt);
    cmd.Mode = ORION_MODE_POSITION;
    cmd.Stabilized = 0;
    cmd.ImpulseTime = 0;

    encodeOrionCmdPacket(&pkt, &cmd);
    return trilliumPktSend(mSockFd, pkt) == 0;
}
