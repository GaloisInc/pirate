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

#include <functional>
#include <mutex>

struct CameraOrientationCallbacks
{
    using GetCallback = std::function<float()>;
    using SetCallback = std::function<void(float)>;
    using UpdateCallback = std::function<void(float)>;

    CameraOrientationCallbacks(
        GetCallback get,
        SetCallback set,
        UpdateCallback update) :
        mGet(get), mSet(set), mUpdate(update)
    {

    }

    GetCallback mGet;
    SetCallback mSet;
    UpdateCallback mUpdate;
};

class OrientationOutput
{
public:
    OrientationOutput();
    virtual ~OrientationOutput();

    virtual int init() = 0;
    virtual void term() = 0;

    virtual float getAngularPosition() = 0;
    virtual void setAngularPosition(float angularPosition) = 0;
    virtual void updateAngularPosition(float positionUpdate) = 0;

    const CameraOrientationCallbacks& getCallbacks();

    static constexpr float DEFAULT_ANG_POS_LIMIT = 90.0;
private:
    const CameraOrientationCallbacks mCallbacks;
};
