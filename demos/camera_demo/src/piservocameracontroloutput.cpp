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

#include <pigpio.h>
#include <unistd.h>
#include <sys/types.h>
#include <cerrno>
#include <iostream>
#include "piservocameracontroloutput.hpp"

PiServoCameraControlOutput::PiServoCameraControlOutput(int servoPin, const Options& options) :
    BaseCameraControlOutput(options),
    mFlip(options.mImageFlip),
    mServoPin(servoPin),
    mGpioLibInit(true)
{

}

PiServoCameraControlOutput::~PiServoCameraControlOutput()
{
    term();
}

int PiServoCameraControlOutput::init()
{
    int rv;

    // pigpio requires root privileges
    if (geteuid() != 0)
    {
        errno = EPERM;
        std::perror("Pi GPIO library requires root privileges");
        return -1;
    }

    if (mGpioLibInit)
    {
        rv = gpioInitialise();
        if (rv < 0)
        {
            std::perror("Failed to initialize the Pi GPIO library");
            return -1;
        }
    }

    rv = gpioSetMode(mServoPin, PI_OUTPUT);
    if (rv != 0)
    {
        std::perror("Failed to set servo pin to output");
        return -1;
    }

    rv = gpioServo(mServoPin, angleToServo(0.0, mFlip));
    if (rv != 0)
    {
        std::perror("Failed to set the initial servo position");
        return -1;
    }

    return 0;
}

void PiServoCameraControlOutput::term()
{
    gpioServo(mServoPin, PI_SERVO_OFF);
    gpioSetMode(mServoPin, PI_INPUT);

    if (mGpioLibInit)
    {
        gpioTerminate();
    }
}

int PiServoCameraControlOutput::angleToServo(float angle, bool flip)
{
    static const float slope =
        (PI_MAX_SERVO_PULSEWIDTH - PI_MIN_SERVO_PULSEWIDTH) /
        (2 * SERVO_ANGLE_LIMIT);
    static const float off = slope * SERVO_ANGLE_LIMIT + PI_MIN_SERVO_PULSEWIDTH;
    if (flip)
    {
        return -1.0 * slope * angle + off;
    }
    else
    {
        return slope * angle + off;
    }
}

bool PiServoCameraControlOutput::equivalentPosition(PanTilt p1, PanTilt p2)
{
    // ignore changes in tilt angle
    return p1.pan == p2.pan;
}

bool PiServoCameraControlOutput::applyAngularPosition(PanTilt angularPosition)
{
    int rv = gpioServo(mServoPin, angleToServo(angularPosition.pan, mFlip));

    if (rv < 0)
    {
        std::perror("Failed to set servo position");
        return false;
    }

    return true;
}
