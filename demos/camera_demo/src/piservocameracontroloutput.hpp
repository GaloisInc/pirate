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

class PiServoCameraControlOutput : public BaseCameraControlOutput
{
public:
    PiServoCameraControlOutput(int servoPin, const Options& options);
    virtual ~PiServoCameraControlOutput();

    virtual int init() override;
    virtual void term() override;

    virtual bool equivalentPosition(PanTilt p1, PanTilt p2) override;

protected:
    virtual bool applyAngularPosition(PanTilt angularPosition) override;

private:
    static int angleToServo(float angle, bool flip);

    const bool mFlip;
    const int mServoPin;
    const bool mGpioLibInit;
    static constexpr float SERVO_ANGLE_LIMIT = 90.0;
};

