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

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
    #include <libavformat/avformat.h>
}

#include "frameprocessor.hpp"

class MpegTsEncoder : public FrameProcessor
{
public:
    MpegTsEncoder(const Options& options);
    virtual ~MpegTsEncoder();

    virtual int init() override;
    virtual void term() override;

protected:
    virtual int process(FrameBuffer data, size_t length) override;

private:
    std::string mH264Url;
    CodecType mCodecType;
    int mFFmpegLogLevel;
    const unsigned mFrameRateNumerator;
    const unsigned mFrameRateDenominator;
    AVCodec *mCodec;
    AVCodecContext *mCodecContext;
    AVFrame *mInputFrame, *mOutputFrame;
    struct SwsContext *mSwsContext;
    AVFormatContext *mOutputContext;
    AVPacket mPkt;

    int processYUYV(FrameBuffer data, size_t length);
    int processH264(FrameBuffer data, size_t length);

    static const AVPixelFormat YUYV_PIXEL_FORMAT = AV_PIX_FMT_YUYV422;
    static const AVPixelFormat H264_PIXEL_FORMAT = AV_PIX_FMT_YUV420P;
};
