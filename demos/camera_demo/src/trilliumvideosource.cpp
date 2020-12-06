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
    mTrilliumUrl(options.mTrilliumUrl),
    mSockFd(-1)
{
}

TrilliumVideoSource::~TrilliumVideoSource()
{
    term();
}

int TrilliumVideoSource::init()
{
    uint8_t octets[4];
    OrionNetworkVideo_t videoSettings;
    OrionPkt_t pktOut;
    std::string decoderUrl = mH264Url;
    std::string host = "";
    int rv, port = 0;

    if (decoderUrl.empty()) {
        std::cerr << "decoder url must be specified on the command-line" << std::endl;
        return 1;
    }
    if (decoderUrl.find("udp://") == 0)
    {
        decoderUrl = decoderUrl.substr(6);
    }

    std::size_t found = decoderUrl.find(':');

    if (found == std::string::npos) {
        std::cerr << "decoder url must be host:port" << std::endl;
        return 1;
    } else {
        host = decoderUrl.substr(0, found);
        port = std::stoi(decoderUrl.substr(found + 1));
    }

    rv = trilliumConnectUDPSocket(mTrilliumUrl, mSockFd);
    if (rv != 0) {
        return rv;
    }

    memset(&videoSettings, 0, sizeof(videoSettings));
    videoSettings.Port = port;
    videoSettings.StreamType = STREAM_TYPE_H264;

    // See https://github.com/trilliumeng/orion-sdk/blob/master/Examples/VideoPlayer/VideoPlayer.c
    if (sscanf(host.c_str(), "%3hhu.%3hhu.%3hhu.%3hhu", &octets[0], &octets[1], &octets[2], &octets[3]) != 4) {
        std::cerr << "decoder url must be ipv4 address" << std::endl;
        return 1;
    }
    int index = 0;
    videoSettings.DestIp = uint32FromBeBytes(octets, &index);

    encodeOrionNetworkVideoPacketStructure(&pktOut, &videoSettings);
    rv = send(mSockFd, (void*) &pktOut, pktOut.Length + ORION_PKT_OVERHEAD, 0);
    if (rv < 0) {
        perror("Trillium video settings send command error");
        return 1;
    }

    return MpegTsDecoder::init();
}

void TrilliumVideoSource::term()
{
    if (mSockFd >= 0) {
        // TODO: can we send a command to stop the camera stream?
        
        close(mSockFd);
    }
}
