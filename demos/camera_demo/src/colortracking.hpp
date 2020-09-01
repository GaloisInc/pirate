#pragma once

#include <thread>

#include "orientationinput.hpp"
#include "frameprocessor.hpp"
#include "options.hpp"

class ColorTracking : public OrientationInput, public FrameProcessor
{
public:
    ColorTracking(const Options& options,
        AngularPosition<float>::UpdateCallback angPosUpdateCallback);
    virtual ~ColorTracking();

    virtual int init() override;
    virtual void term() override;

private:
    const float          mAngIncrement;
    const bool           mImageSlidingWindow;
    const unsigned char  mImageTrackingRGB[3];

    void computeTracking(int* x_pos, int *y_pos, FrameBuffer data);

protected:
    virtual int processFrame(FrameBuffer data, size_t length) override;
    virtual unsigned char* getFrame(unsigned index, VideoType videoType) override;

};
