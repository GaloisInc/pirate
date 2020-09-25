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

#include <memory>
#include <thread>
#include <vector>

#include "orientationoutput.hpp"
#include "options.hpp"
#include "camerademo-serialization.hpp"

class RemoteOrientationOutput : public OrientationOutput
{
public:
    RemoteOrientationOutput(
        RemoteOutputType remoteType,
        std::unique_ptr<OrientationOutput> delegate,
        const Options& options);
    virtual ~RemoteOrientationOutput();

    virtual int init() override;
    virtual void term() override;

    virtual float getAngularPosition() override;
    virtual void setAngularPosition(float angularPosition) override;
    virtual void updateAngularPosition(float positionUpdate) override;

private:
    const std::unique_ptr<OrientationOutput> mDelegate;

    int mClientReadGd;
    int mClientWriteGd;
    CameraDemo::OrientationOutputReqSender mClientSenderId;

    int mServerReadGd;
    int mServerWriteColorTrackingGd;
    int mServerWriteXWinFrameGd;

    std::mutex mClientLock;
    std::thread *mPollThread;
    bool mPoll;

    void pollThread();

    bool sendRequest(const CameraDemo::OrientationOutputRequest& request);
    bool sendResponse(CameraDemo::OrientationOutputReqSender id,
        const CameraDemo::OrientationOutputResponse& response);

    bool recvRequest(CameraDemo::OrientationOutputRequest& request);
    bool recvResponse(CameraDemo::OrientationOutputResponse& response);
};
