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

#pragma once

#include "baseorientationoutput.hpp"
#include "orion-sdk/OrionPublicPacketShim.hpp"

class TrilliumControl : public BaseOrientationOutput
{
public:
    TrilliumControl(const Options& options);
    virtual ~TrilliumControl();

    virtual int init() override;
    virtual void term() override;

protected:
    virtual bool applyAngularPosition(PanTilt angularPosition) override;

private:
    const std::string mTrilliumIpAddress;
    int mSockFd;

    const bool mVerbose;
    std::thread *mReceiveThread;
    bool mReceive;
    void reveiveThread();
    static void processTrilliumPacket(OrionPkt_t& pkt);
    static void processSoftwareDiagnostics(OrionPkt_t& pkt);
    static void processGeolocateTelemetry(OrionPkt_t& pkt);
};
