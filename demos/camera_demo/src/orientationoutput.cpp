#include <iomanip>
#include <iostream>
#include "orientationoutput.hpp"

OrientationOutput::OrientationOutput(float angularPositionLimit, bool verbose) :
    AngularPosition(-angularPositionLimit, angularPositionLimit),
    mVerbose(verbose),
    mUpdateCallback(std::bind(&OrientationOutput::setAngularPosition, this,
                std::placeholders::_1))
{

}

OrientationOutput::~OrientationOutput()
{
    
}

int OrientationOutput::init()
{
    return 0;
}

void OrientationOutput::term()
{

}

bool OrientationOutput::setAngularPosition(float angularPosition)
{
    if (mVerbose)
    {
        std::cout   << "Camera Position "
                    << std::setprecision(4)
                    << angularPosition << std::endl;
    }

    return AngularPosition::setAngularPosition(angularPosition);
}

const OrientationUpdateCallback& OrientationOutput::getUpdateCallback()
{
    return mUpdateCallback;
}
