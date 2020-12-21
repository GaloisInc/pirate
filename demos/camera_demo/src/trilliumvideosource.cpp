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

#include <string>
#include <iostream>

#include "options.hpp"
#include "trilliumvideosource.hpp"
#include "trilliumutilities.hpp"

#include "orion-sdk/OrionPublicPacket.hpp"
#include "orion-sdk/OrionPublicPacketShim.hpp"
#include "orion-sdk/Constants.hpp"
#include "orion-sdk/OrionComm.hpp"
#include "orion-sdk/fielddecode.hpp"

TrilliumVideoSource::TrilliumVideoSource(const Options& options,
        const std::vector<std::shared_ptr<FrameProcessor>>& frameProcessors) :
    MpegTsDecoder(options, frameProcessors),
    mTrilliumIpAddress(options.mTrilliumIpAddress),
    mSockFd(-1)
{
}

TrilliumVideoSource::~TrilliumVideoSource()
{
    term();
}

int TrilliumVideoSource::init()
{
    int rv = -1;
    std::string host = "";
    uint32_t host_addr = 0;
    short port = 0;
    std::string decoderUrl = mH264Url;
    OrionNetworkVideo_t videoSettings;
    OrionPkt_t pkt;

    if (decoderUrl.empty())
    {
        std::cerr << "decoder url must be specified on the command-line" << std::endl;
        return 1;
    }

    if (decoderUrl.find("udp://") == 0)
    {
        decoderUrl = decoderUrl.substr(6);
    }

    std::size_t found = decoderUrl.find(':');

    if (found == std::string::npos)
    {
        std::cerr << "decoder url must be host:port" << std::endl;
        return 1;
    }
    else
    {
        host = decoderUrl.substr(0, found);
        port = std::stoi(decoderUrl.substr(found + 1));
    }

    rv = inet_pton(AF_INET, host.c_str(), &host_addr);
    if (rv != 1)
    {
        std::cerr << "Invalid address format: " << host << std::endl;
        return -1;
    }

    // Video stream enable command and packet
    videoSettings.DestIp        = ntohl(host_addr);
    videoSettings.Port          = port;
    videoSettings.Bitrate       = 0;
    videoSettings.Ttl           = 0;
    videoSettings.StreamType    = STREAM_TYPE_H264;
    videoSettings.MjpegQuality  = 0;
    videoSettings.SaveSettings  = 0;
    videoSettings.TsPacketCount = 0;
    encodeOrionNetworkVideoPacketStructure(&pkt, &videoSettings);

    // Prepare Trillum UDP command socket
    rv = trilliumConnectUDPSocket(mTrilliumIpAddress, mSockFd);
    if (rv != 0)
    {
        return rv;
    }

    // Trillium needs to be told twice to start the video stream,
    // especially after powerup.
    // A proper approach involves sending multiple requests
    // until a response is received. Since camera commands are
    // one-way in some configurations, we cannot rely on receiving
    // feedback from the camera.

    for (int i = 0; i < 2; i++)
    {
        rv = trilliumPktSend(mSockFd, pkt);
        if (rv != 0)
        {
            return rv;
        }

        usleep(1000);
    }

    // Trillum UDP command socket is no longer needed
    close(mSockFd);
    mSockFd = -1;

    return MpegTsDecoder::init();
}

void TrilliumVideoSource::term()
{
    if (mSockFd >= 0) {
        // TODO: can we send a command to stop the camera stream?
        
        close(mSockFd);
    }
}
