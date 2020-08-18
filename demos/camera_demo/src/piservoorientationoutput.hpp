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

