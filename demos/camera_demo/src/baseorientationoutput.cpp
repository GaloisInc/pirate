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

#include <iomanip>
#include <iostream>
#include <functional>
#include "baseorientationoutput.hpp"

BaseOrientationOutput::BaseOrientationOutput(const Options& options) :
    OrientationOutput(),
    mVerbose(options.mVerbose),
    mAngularPositionMin(options.mAngularPositionMin),
    mAngularPositionMax(options.mAngularPositionMax),
    mAngularPosition(0.0)
{

}

BaseOrientationOutput::~BaseOrientationOutput()
{

}

int BaseOrientationOutput::init()
{
    return 0;
}

void BaseOrientationOutput::term()
{

}

float BaseOrientationOutput::getAngularPosition()
{
    float angularPosition;
    mLock.lock();
    angularPosition = mAngularPosition;
    mLock.unlock();
    return angularPosition;
}

bool BaseOrientationOutput::safelySetAngularPosition(float& angularPosition)
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

void BaseOrientationOutput::setAngularPosition(float angularPosition)
{
    mLock.lock();

    bool updated = safelySetAngularPosition(angularPosition);
    if (updated)
    {
        updated = applyAngularPosition(angularPosition);
    }

    if (mVerbose)
    {
        std::cout   << "Camera Position Set "
                    << std::setprecision(4)
                    << angularPosition << std::endl;
    }

    mLock.unlock();
}

void BaseOrientationOutput::updateAngularPosition(float positionUpdate)
{
    mLock.lock();

    float angularPosition = mAngularPosition;
    angularPosition += positionUpdate;

    bool updated = safelySetAngularPosition(angularPosition);

    if (updated)
    {
        updated = applyAngularPosition(angularPosition);
    }

    if (mVerbose)
    {
        std::cout   << "Camera Position Update "
                    << std::setprecision(4) << positionUpdate
                    << " to "
                    << std::setprecision(4) << angularPosition
                    << std::endl;
    }

    mLock.unlock();
}

bool BaseOrientationOutput::applyAngularPosition(float angularPosition)
{
    (void) angularPosition;
    return true;
}
