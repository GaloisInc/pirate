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

// Definition of a simple interface for reading camera angular orientation

#include "orientation.hpp"

class OrientationInput : public AngularPosition<float>
{
public:
    OrientationInput(   AngularPosition<float>::UpdateCallback updateCallback,
                        float angPosMin = -AngularPosition<float>::DEFAULT_ANG_POS_LIMIT, 
                        float angPosMax =  AngularPosition<float>::DEFAULT_ANG_POS_LIMIT) :
        AngularPosition(angPosMin, angPosMax),
        mUpdateCallback(updateCallback)
    {

    }

    virtual ~OrientationInput()
    {

    }

    virtual int init() = 0;
    virtual void term() = 0;

    virtual bool setAngularPosition(float angularPosition) override
    {
        bool updated = AngularPosition::setAngularPosition(angularPosition);
        if (updated)
        {
            mUpdateCallback(getAngularPosition());
        }

        return updated;
    }
private:
    AngularPosition<float>::UpdateCallback mUpdateCallback;
};

