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

#include "basecameracontroloutput.hpp"
#include "orion-sdk/OrionPublicPacketShim.hpp"

class TrilliumControl : public BaseCameraControlOutput
{
public:
    TrilliumControl(const Options& options);
    virtual ~TrilliumControl();

    virtual int init() override;
    virtual void term() override;

protected:
    virtual bool applyAngularPosition(PanTilt angularPosition) override;
    virtual void updateZoom(CameraZoom zoom) override;

private:
    const std::string mTrilliumIpAddress;
    const std::string mTrilliumConfig;
    int mSockFd;

    const bool mVerbose;
    std::thread *mReceiveThread;
    bool mReceive;
    int trilliumSensorConfig();
    int trilliumConfigParse(std::string file, OrionAptinaSettings_t &cfg);
    int trilliumLimitsRequest();
    void reveiveThread();
    void processTrilliumPacket(OrionPkt_t& pkt);
    void processSoftwareDiagnostics(OrionPkt_t& pkt);
    void processCameraState(OrionPkt_t& pkt);
    void printCameraStatus();

    struct {
        GeolocateTelemetryCore_t mGeo;
        GpsData_t mGps;
        OrionAptinaSettings_t mAptina;
        OrionLimitsData_t mLimits;
        float mZoom;
        float mFocus;
        OrionDiagnostics_t mDiag;
        OrionSoftwareDiagnostics_t mSoftDiad[BOARD_COUNT]; 
        struct {
            unsigned mID;
            unsigned mLength;
        } mLastPacket;
        unsigned mPacketCount;
    } mState;

    static constexpr float mZoomIncrement = 0.1;
    static constexpr float mZoomMin = 1.0;
    static constexpr float mZoomMax = 8.0;
    static constexpr float mZoomDefault = mZoomMin;

    const bool mFlip;
};
