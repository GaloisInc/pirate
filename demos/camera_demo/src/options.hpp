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

enum VideoType { VIDEO_JPEG, VIDEO_YUYV, VIDEO_H264, VIDEO_BGRX, VIDEO_STREAM };
enum CodecType { CODEC_MPEG1, CODEC_MPEG2, CODEC_H264 };
enum InputType { Freespace, Keyboard };
enum FrameProcessorType { Filesystem, XWindows, H264Stream };
enum OutputType { PiServo, Print };

using FrameBuffer = const unsigned char *;

// Command-line options
struct Options
{
    Options() :
        mVideoDevice("/dev/video0"),
        mVideoInputType(VIDEO_JPEG),
        mVideoOutputType(VIDEO_JPEG),
        mEncoderCodecType(CODEC_H264),
        mImageWidth(640),
        mImageHeight(480),
        mImageHorizontalFlip(false),
        mImageVerticalFlip(false),
        mImageSlidingWindow(false),
        mImageTracking(false),
        mImageTrackingRGB{0, 0, 0},
        mImageTrackingThreshold(2048),
        mFrameRateNumerator(1),
        mFrameRateDenominator(30),
        mImageOutputDirectory("/tmp"),
        mImageOutputMaxFiles(100),
        mOutputType(PiServo),
        mInputKeyboard(false),
        mInputFreespace(false),
        mFilesystemProcessor(false),
        mXWinProcessor(false),
        mH264Encoder(false),
        mH264EncoderUrl(""),
        mH264DecoderUrl(""),
        mAngularPositionMin(-45.0),
        mAngularPositionMax(45.0),
        mVerbose(false),
        mFFmpegLogLevel(8 /*AV_LOG_FATAL*/),
        mClientId(0),
        mOutputClientChannel("udp_socket,127.0.0.1,22660"),
        mOutputServerChannel("udp_socket,127.0.0.1,22661"),
        mClientReadGd(-1),
        mClientWriteGd(-1),
        mServerReadGd(-1),
        mServerWriteGds()
    {

    }

    std::string mVideoDevice;
    VideoType mVideoInputType;
    VideoType mVideoOutputType;
    CodecType mEncoderCodecType;
    unsigned mImageWidth;
    unsigned mImageHeight;
    bool mImageHorizontalFlip;
    bool mImageVerticalFlip;
    bool mImageSlidingWindow;
    bool mImageTracking;
    unsigned char mImageTrackingRGB[3];
    unsigned mImageTrackingThreshold;
    const unsigned mFrameRateNumerator;
    const unsigned mFrameRateDenominator;
    std::string mImageOutputDirectory;
    unsigned mImageOutputMaxFiles;
    OutputType mOutputType;
    bool mInputKeyboard;
    bool mInputFreespace;
    bool mFilesystemProcessor;
    bool mXWinProcessor;
    bool mH264Encoder;
    std::string mH264EncoderUrl;
    std::string mH264DecoderUrl;
    float mAngularPositionMin;
    float mAngularPositionMax;
    bool mVerbose;
    int mFFmpegLogLevel;
    int mClientId;
    std::string mOutputClientChannel;
    std::string mOutputServerChannel;

    int mClientReadGd;
    int mClientWriteGd;
    int mServerReadGd;
    std::vector<int> mServerWriteGds;
};
