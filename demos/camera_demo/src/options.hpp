#pragma once

#include <string>

enum VideoType { JPEG, YUYV, RGBX };
enum InputType { Freespace, Keyboard };
enum FrameProcessorType { Filesystem, XWindows, H264 };
enum OutputType { PiServo, Print };

using FrameBuffer = const unsigned char *;

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
        mImageSlidingWindow(false),
        mImageTracking(false),
        mImageTrackingRGB{0, 0, 0},
        mImageTrackingThreshold(2048),
        mFrameRateNumerator(1),
        mFrameRateDenominator(1),
        mImageOutputDirectory("/tmp"),
        mImageOutputMaxFiles(100),
        mOutputType(PiServo),
        mInputType(Freespace),
        mFilesystemProcessor(false),
        mXWinProcessor(false),
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
    bool mImageSlidingWindow;
    bool mImageTracking;
    unsigned char mImageTrackingRGB[3];
    unsigned mImageTrackingThreshold;
    unsigned mFrameRateNumerator;
    unsigned mFrameRateDenominator;
    std::string mImageOutputDirectory;
    unsigned mImageOutputMaxFiles;
    OutputType mOutputType;
    InputType mInputType;
    bool mFilesystemProcessor;
    bool mXWinProcessor;
    float mAngularPositionLimit;
    bool mVerbose;
};
