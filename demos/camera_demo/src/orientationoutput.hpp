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

#include <mutex>
#include "cameraorientation.hpp"

class OrientationOutput : public CameraOrientation
{
public:
    OrientationOutput(float angularPositionLimit = DEFAULT_ANG_POS_LIMIT,
                        bool verbose = false);
    virtual ~OrientationOutput();

    virtual int init();
    virtual void term();

    virtual float getAngularPosition() override;
    virtual bool setAngularPosition(float& angularPosition) override;
    virtual bool updateAngularPosition(float positionUpdate) override;

    const CameraOrientationCallbacks& getCallbacks();
protected:
    const bool mVerbose;

    virtual bool applyAngularPosition(float angularPosition);
private:
    std::mutex mLock;
    static constexpr float DEFAULT_ANG_POS_LIMIT = 90.0;

    const CameraOrientationCallbacks mCallbacks;
};

