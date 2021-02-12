/*
 * This work was authored by Two Six Labs, LLC and is sponsored by a subcontract
 * agreement with Galois, Inc.  This material is based upon work supported by
 * the Defense Advanced Research Projects Agency (DARPA) under Contract No.
 * HR0011-19-C-0103.
 *
 * The Government has unlimited rights to use, modify, reproduce, release,
 * perform, display, or disclose computer software or computer software
 * documentation marked with this legend. Any reproduction of technical data,
 * computer software, or portions thereof marked with this legend must also
 * reproduce this marking.
 *
 * Copyright 2020 Two Six Labs, LLC.  All rights reserved.
 */

#pragma once

#include <string>
#include <vector>

enum VideoType { VIDEO_JPEG, VIDEO_YUYV, VIDEO_H264, VIDEO_BGRX, VIDEO_STREAM, VIDEO_TRILLIUM, VIDEO_TEST, VIDEO_NULL };
enum CodecType { CODEC_MPEG1, CODEC_MPEG2, CODEC_H264 };
enum InputType { Freespace, Keyboard };
enum FrameProcessorType { Filesystem, XWindows, H264Stream, MetaDataProcessor };
enum DataStreamType { VideoData, MetaData };
enum OutputType { PiServoOutput, TrilliumOutput, PrintOutput, NoneOutput };

using FrameBuffer = const unsigned char *;

// Command-line options
struct Options
{
    Options() :
        mVideoDevice("/dev/video0"),
        mVideoInputType(VIDEO_NULL),
        mVideoOutputType(VIDEO_NULL),
        mEncoderCodecType(CODEC_H264),
        mImageWidth(640),
        mImageHeight(480),
        mImageFlip(false),
        mImageSlidingWindow(false),
        mImageTracking(false),
        mImageTrackingRGB{0, 0, 0},
        mImageTrackingThreshold(2048),
        mImageColorPick(false),
        mFrameRateNumerator(1),
        mFrameRateDenominator(30),
        mImageOutputDirectory("/tmp"),
        mImageOutputMaxFiles(100),
        mOutputType(NoneOutput),
        mInputKeyboard(false),
        mInputFreespace(false),
        mFilesystemProcessor(false),
        mXWinProcessor(false),
        mMetaDataProcessor(false),
        mH264Encoder(false),
        mH264EncoderUrl(""),
        mH264DecoderUrl(""),
        mTrilliumIpAddress(""),
        mTrilliumConfig(""),
        mPanAxisMin(-45.0),
        mPanAxisMax(45.0),
        mTiltAxisMin(-45.0),
        mTiltAxisMax(45.0),
        mAngularPositionIncrement(1.0),
        mVerbose(false),
        mGDB(false),
        mFFmpegLogLevel(8 /*AV_LOG_FATAL*/),
        mHasInput(false),
        mHasOutput(false),
        mGapsRequestChannel(),
        mGapsResponseChannel()
    {

    }

    std::string mVideoDevice;
    VideoType mVideoInputType;
    VideoType mVideoOutputType;
    CodecType mEncoderCodecType;
    unsigned mImageWidth;
    unsigned mImageHeight;
    bool mImageFlip;
    bool mImageSlidingWindow;
    bool mImageTracking;
    unsigned char mImageTrackingRGB[3];
    unsigned mImageTrackingThreshold;
    bool mImageColorPick;
    const unsigned mFrameRateNumerator;
    const unsigned mFrameRateDenominator;
    std::string mImageOutputDirectory;
    unsigned mImageOutputMaxFiles;
    OutputType mOutputType;
    bool mInputKeyboard;
    bool mInputFreespace;
    bool mFilesystemProcessor;
    bool mXWinProcessor;
    bool mMetaDataProcessor;
    bool mH264Encoder;
    std::string mH264EncoderUrl;
    std::string mH264DecoderUrl;
    std::string mTrilliumIpAddress;
    std::string mTrilliumConfig;
    float mPanAxisMin;
    float mPanAxisMax;
    float mTiltAxisMin;
    float mTiltAxisMax;
    float mAngularPositionIncrement;
    bool mVerbose;
    bool mGDB;
    int mFFmpegLogLevel;
    bool mHasInput;
    bool mHasOutput;
    std::vector<std::string> mGapsRequestChannel;
    std::vector<std::string> mGapsResponseChannel;
};
