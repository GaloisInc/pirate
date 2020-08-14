#pragma once

#include "orientationoutput.hpp"
#include "piservoorientationoutput.hpp"

class OrientationOutputCreator
{
private:
    static constexpr int PI_SERVO_PIN = 27;

public:
    enum OutputType { PiServo, Print };

    static OrientationOutput * get(OutputType outputType, float angPosLimit)
    {
        switch (outputType)
        {
#ifdef PIGPIO_PRESENT
            case PiServo:
                return new PiServoOrientationOutput(PI_SERVO_PIN, angPosLimit);
#endif
            case Print:
            default:
                return new OrientationOutput(angPosLimit);
        }

    }
};
