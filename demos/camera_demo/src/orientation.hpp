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

template <class T>
class AngularPosition
{
public:
    AngularPosition(T angPosMin = -DEFAULT_ANG_POS_LIMIT,
                    T angPosMax =  DEFAULT_ANG_POS_LIMIT) :
        mAngularPositionMin(angPosMin),
        mAngularPositionMax(angPosMax),
        mAngularPosition(0)
    {

    }

    virtual ~AngularPosition()
    {

    }

    virtual T getAngularPosition()
    {
        return mAngularPosition;   
    }

    virtual bool setAngularPosition(T& angularPosition)
    {
        if (angularPosition < mAngularPositionMin)
        {
            angularPosition = mAngularPositionMin;
        }
        else if (angularPosition > mAngularPositionMax)
        {
            angularPosition = mAngularPositionMax;
        }

        if (mAngularPosition != angularPosition)
        {
            mAngularPosition = angularPosition;
            return true;
        }

        return false;
    }

    virtual bool updateAngularPosition(T positionUpdate)
    {
        T angularPosition = mAngularPosition + positionUpdate;
        return setAngularPosition(angularPosition);
    }

    static constexpr T DEFAULT_ANG_POS_LIMIT = 90;

    using GetCallback = std::function<T()>;
    using SetCallback = std::function<bool(T&)>;
    using UpdateCallback = std::function<bool(T)>;
    const T mAngularPositionMin;
    const T mAngularPositionMax;
private:
    T mAngularPosition;
};

