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

enum VideoType { VIDEO_JPEG, VIDEO_YUYV, VIDEO_H264, VIDEO_BGRX, VIDEO_STREAM };
enum CodecType { CODEC_MPEG1, CODEC_MPEG2, CODEC_H264 };
enum InputType { Freespace, Keyboard };
enum FrameProcessorType { Filesystem, XWindows, H264Stream };
enum OutputType { PiServo, Print };
enum RemoteOutputType { OutputServer, OutputTrackingClient, OutputXWindowsClient };

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
        mImageTrackingChannel("udp_socket,127.0.0.1,22661"),
        mImageTrackingRGB{0, 0, 0},
        mImageTrackingThreshold(2048),
        mFrameRateNumerator(1),
        mFrameRateDenominator(30),
        mImageOutputDirectory("/tmp"),
        mImageOutputMaxFiles(100),
        mOutputType(PiServo),
        mOutputChannel("udp_socket,127.0.0.1,22660"),
        mInputKeyboard(false),
        mInputFreespace(false),
        mFilesystemProcessor(false),
        mXWinProcessor(false),
        mXWinProcessorChannel("udp_socket,127.0.0.1,22662"),
        mH264Encoder(false),
        mH264EncoderUrl(""),
        mH264DecoderUrl(""),
        mAngularPositionMin(-45.0),
        mAngularPositionMax(45.0),
        mVerbose(false),
        mFFmpegLogLevel(8 /*AV_LOG_FATAL*/),
        mClientTrackingReadGd(-1),
        mClientTrackingWriteGd(-1),
        mClientXWinReadGd(-1),
        mClientXWinWriteGd(-1),
        mClientWriteGd(-1),
        mServerReadGd(-1),
        mServerWriteTrackingGd(-1),
        mServerWriteXWinGd(-1)
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
    std::string mImageTrackingChannel;
    unsigned char mImageTrackingRGB[3];
    unsigned mImageTrackingThreshold;
    const unsigned mFrameRateNumerator;
    const unsigned mFrameRateDenominator;
    std::string mImageOutputDirectory;
    unsigned mImageOutputMaxFiles;
    OutputType mOutputType;
    std::string mOutputChannel;
    bool mInputKeyboard;
    bool mInputFreespace;
    bool mFilesystemProcessor;
    bool mXWinProcessor;
    std::string mXWinProcessorChannel;
    bool mH264Encoder;
    std::string mH264EncoderUrl;
    std::string mH264DecoderUrl;
    float mAngularPositionMin;
    float mAngularPositionMax;
    bool mVerbose;
    int mFFmpegLogLevel;

    int mClientTrackingReadGd;
    int mClientTrackingWriteGd;
    int mClientXWinReadGd;
    int mClientXWinWriteGd;
    int mClientWriteGd;

    int mServerReadGd;
    int mServerWriteTrackingGd;
    int mServerWriteXWinGd;
};
