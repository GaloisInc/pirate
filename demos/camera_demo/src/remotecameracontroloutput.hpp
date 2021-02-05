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

#include "cameracontroloutput.hpp"
#include "options.hpp"
#include "remotes.hpp"
#include "camerademo-serialization.hpp"

class RemoteCameraControlOutput : public CameraControlOutput
{
public:
    RemoteCameraControlOutput(
        std::unique_ptr<CameraControlOutput> delegate,
        const Options& options,
        const RemoteDescriptors& remotes);
    virtual ~RemoteCameraControlOutput();

    virtual int init() override;
    virtual void term() override;

    virtual PanTilt getAngularPosition() override;
    virtual void setAngularPosition(PanTilt angularPosition) override;
    virtual void updateAngularPosition(PanTilt positionUpdate) override;
    virtual bool equivalentPosition(PanTilt p1, PanTilt p2) override;
    virtual void updateZoom(CameraZoom zoom) override;

private:
    const std::unique_ptr<CameraControlOutput> mDelegate;
    uint16_t mMessageCounter;

    const bool mHasOutput;
    const int mGapsRequestWriteGd;
    const int mGapsResponseReadGd;
    const std::vector<int> mGapsRequestReadGds;
    const std::vector<int> mGapsResponseWriteGds;

    std::mutex mClientLock;
    std::thread *mPollThread;
    bool mPoll;

    void pollThread();

    bool sendRequest(const CameraDemo::CameraControlOutputRequest& request);
    bool sendResponse(uint16_t id, const CameraDemo::CameraControlOutputResponse& response);

    int recvRequest(CameraDemo::CameraControlOutputRequest& request, int& clientId);
    bool recvResponse(CameraDemo::CameraControlOutputResponse& response);
};
