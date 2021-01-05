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

#include "pantilt.hpp"
#include "camerazoom.hpp"

#include <functional>
#include <mutex>

struct CameraControlCallbacks
{
    using PosGet = std::function<PanTilt()>;
    using PosSet = std::function<void(PanTilt)>;
    using PosUpdate = std::function<void(PanTilt)>;
    using ZoomUpdate = std::function<void(CameraZoom)>;

    CameraControlCallbacks(
        PosGet posGet,
        PosSet posSet,
        PosUpdate posUpdate,
        ZoomUpdate zoomUpdate) :
        mPosGet(posGet), mPosSet(posSet), mPosUpdate(posUpdate),
        mZoomUpdate(zoomUpdate)
    {

    }

    PosGet mPosGet;
    PosSet mPosSet;
    PosUpdate mPosUpdate;
    ZoomUpdate mZoomUpdate;
};

class CameraControlOutput
{
public:
    CameraControlOutput();
    virtual ~CameraControlOutput();

    virtual int init() = 0;
    virtual void term() = 0;

    virtual PanTilt getAngularPosition() = 0;
    virtual void setAngularPosition(PanTilt angularPosition) = 0;
    virtual void updateAngularPosition(PanTilt positionUpdate) = 0;
    virtual bool equivalentPosition(PanTilt p1, PanTilt p2) = 0;

    virtual void updateZoom(CameraZoom zoom) = 0;

    const CameraControlCallbacks& getCallbacks();

    static constexpr float DEFAULT_ANG_POS_LIMIT = 90.0;
private:
    const CameraControlCallbacks mCallbacks;
};
