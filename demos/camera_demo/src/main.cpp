#include <string>
#include <iostream>
#include <functional>

#include "freespaceorientationinput.hpp"
#include "keyboardorientationinput.hpp"
#include "piservoorientationoutput.hpp"
#include "videosensor.hpp"
#include "fileframeprocessor.hpp"

// Debug
#include <chrono>
#include <thread>

int main()
{
    int rv;
    const float angPosLim = 45.0;
    const int piServoPin = 27;

    OrientationOutput * orientationOutput = new PiServoOrientationOutput(
            piServoPin, angPosLim);
    
    OrientationInput * orientationInput = new FreespaceOrientationInput(
            orientationOutput->getUpdateCallback(), -angPosLim, angPosLim);
        
    FrameProcessor * frameProcessor = new FileFrameProcessor();
    VideoSensor * videoSensor = new VideoSensor(
            frameProcessor->getProcessFrameCallback());

    rv = orientationInput->init();
    if (rv != 0)
    {
        return -1;
    }

    rv = orientationOutput->init();
    if (rv != 0)
    {
        return -1;
    }
    
    rv = videoSensor->init();
    if (rv != 0)
    {
        videoSensor->term();
        return -1;
    }

    rv = frameProcessor->init();
    if (rv != 0)
    {
        return -1;
    }

    rv = videoSensor->captureEnable(true);
    if (rv != 0)
    {
        return -1;
    }

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    
    rv = videoSensor->captureEnable(false);
    if (rv != 0)
    {
        return -1;
    }

    delete orientationInput;
    delete orientationOutput;
    delete videoSensor;
    delete frameProcessor;
    return 0;
}
