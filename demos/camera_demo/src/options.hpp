#pragma once

#include <string>

enum VideoType { JPEG, YUYV };
enum InputType { Freespace, Keyboard };
enum FrameProcessorType { Filesystem, XWindows };
enum OutputType { PiServo, Print };

// Command-line options
struct Options
{
    Options() :
        mVideoDevice("/dev/video0"),
        mVideoType(JPEG),
        mImageWidth(640),
        mImageHeight(480),
        mImageHorizontalFlip(false),
        mImageVerticalFlip(false),
        mImageMonochrome(false),
        mFrameRateNumerator(1),
        mFrameRateDenominator(1),
        mImageOutputDirectory("/tmp"),
        mOutputType(PiServo),
        mInputType(Freespace),
        mProcessorType(Filesystem),
        mAngularPositionLimit(45.0),
        mVerbose(false)
    {

    }

    std::string mVideoDevice;
    VideoType mVideoType;
    unsigned mImageWidth;
    unsigned mImageHeight;
    bool mImageHorizontalFlip;
    bool mImageVerticalFlip;
    bool mImageMonochrome;
    unsigned mFrameRateNumerator;
    unsigned mFrameRateDenominator;
    std::string mImageOutputDirectory;
    OutputType mOutputType;
    InputType mInputType;
    FrameProcessorType mProcessorType;
    float mAngularPositionLimit;
    bool mVerbose;
};
