#pragma once

#include "options.hpp"
#include "orientationoutput.hpp"
#include "piservoorientationoutput.hpp"

class OrientationOutputCreator
{
private:
    static constexpr int PI_SERVO_PIN = 27;

public:
    static OrientationOutput * get(Options& options)
    {
        switch (options.mOutputType)
        {
#ifdef PIGPIO_PRESENT
            case PiServo:
                return new PiServoOrientationOutput(PI_SERVO_PIN,
                    options.mAngularPositionLimit, options.mVerbose);
#endif
            case Print:
            default:
                return new OrientationOutput(options.mAngularPositionLimit, options.mVerbose);
        }

    }
};
