#pragma once

#include "orientationoutput.hpp"

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
#if 0
            case PiServo:
                return new PiServoOrientationOutput(PI_SERVO_PIN, angPosLimit);
#endif
            case Print:
            default:
                return new OrientationOutput(angPosLimit);
        }

    }
};
