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

class PiServoOrientationOutput : public OrientationOutput
{
public:
    PiServoOrientationOutput(int servoPin, float angLimit = DEFAULT_ANGLE_LIMIT,
            bool verbose = false, bool gpioLibInit = true);
    virtual ~PiServoOrientationOutput();

    virtual int init() override;
    virtual void term() override;

    virtual bool setAngularPosition(float angularPosition) override;

private:
    static int angleToServo(float angle);

    const int mServoPin;
    const bool mGpioLibInit;
    static constexpr float DEFAULT_ANGLE_LIMIT = 30.0;
    static constexpr float SERVO_ANGLE_LIMIT = 90.0;
};

