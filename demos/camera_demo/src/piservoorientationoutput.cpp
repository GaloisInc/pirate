#include <pigpio.h>
#include <unistd.h>
#include <sys/types.h>
#include <cerrno>
#include <iostream>
#include "piservoorientationoutput.hpp"

PiServoOrientationOutput::PiServoOrientationOutput(int servoPin, float angLimit,
        bool verbose, bool gpioLibInit) :
    OrientationOutput(angLimit, verbose),
    mServoPin(servoPin),
    mGpioLibInit(gpioLibInit)

{

}

PiServoOrientationOutput::~PiServoOrientationOutput()
{
    term();
}

int PiServoOrientationOutput::init()
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

    rv = gpioServo(mServoPin, angleToServo(0.0));
    if (rv != 0)
    {
        std::perror("Failed to set the initial servo position");
        return -1;
    }

    return 0;
}

void PiServoOrientationOutput::term()
{
    gpioServo(mServoPin, PI_SERVO_OFF);
    gpioSetMode(mServoPin, PI_INPUT);

    if (mGpioLibInit)
    {
        gpioTerminate();
    }
}

bool PiServoOrientationOutput::setAngularPosition(float angularPosition)
{
    OrientationOutput::setAngularPosition(angularPosition);
    int rv = gpioServo(mServoPin, angleToServo(getAngularPosition()));

    if (rv < 0)
    {
        std::perror("Failed to set servo position");
        return false;
    }

    return true;
}

int PiServoOrientationOutput::angleToServo(float angle)
{
    static const float slope =
        (PI_MAX_SERVO_PULSEWIDTH - PI_MIN_SERVO_PULSEWIDTH) /
        (2 * SERVO_ANGLE_LIMIT);
    static const float off = slope * SERVO_ANGLE_LIMIT + PI_MIN_SERVO_PULSEWIDTH;
    return slope * angle + off;
}
