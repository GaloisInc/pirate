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

#include "options.hpp"
#include "cameracontroloutput.hpp"
#include "piservocameracontroloutput.hpp"
#include "trilliumcontrol.hpp"

class CameraControlOutputCreator
{
private:
    static constexpr int PI_SERVO_PIN = 27;

public:
    static CameraControlOutput * get(const Options& options)
    {
        switch (options.mOutputType)
        {
#ifdef PIGPIO_PRESENT
            case PiServoOutput:
                return new PiServoCameraControlOutput(PI_SERVO_PIN, options);
#endif
            case TrilliumOutput:
                return new TrilliumControl(options);
            case PrintOutput:
            case NoneOutput:
            default:
                return new BaseCameraControlOutput(options);
        }
    }
};
