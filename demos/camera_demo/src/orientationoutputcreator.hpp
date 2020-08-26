#pragma once

#include "orientationoutput.hpp"
#include "piservoorientationoutput.hpp"

class OrientationOutputCreator
{
private:
    static constexpr int PI_SERVO_PIN = 27;

public:
    static OrientationOutput * get(OutputType outputType, float angPosLimit,
        bool verbose)
    {
        switch (outputType)
        {
#ifdef PIGPIO_PRESENT
            case PiServo:
                return new PiServoOrientationOutput(PI_SERVO_PIN, angPosLimit,
                    verbose);
#endif
            case Print:
            default:
                return new OrientationOutput(angPosLimit, verbose);
        }

    }
};
