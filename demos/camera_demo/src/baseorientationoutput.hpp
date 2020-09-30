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

#include "orientationoutput.hpp"
#include "options.hpp"

class BaseOrientationOutput : public OrientationOutput
{
public:
    BaseOrientationOutput(const Options& options);
    virtual ~BaseOrientationOutput();

    virtual int init() override;
    virtual void term() override;

    virtual float getAngularPosition() override;
    virtual void setAngularPosition(float angularPosition) override;
    virtual void updateAngularPosition(float positionUpdate) override;

    const bool mVerbose;
    const float mAngularPositionMin;
    const float mAngularPositionMax;

protected:
    virtual bool applyAngularPosition(float angularPosition);

private:
    std::mutex mLock;
    float mAngularPosition;

    bool safelySetAngularPosition(float& angularPosition);
};
