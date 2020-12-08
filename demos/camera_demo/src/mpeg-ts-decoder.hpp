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

#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <stdint.h>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
    #include <libavformat/avformat.h>
}

#include "imageconvert.hpp"
#include "frameprocessor.hpp"
#include "options.hpp"
#include "videosource.hpp"

class MpegTsDecoder : public VideoSource
{
public:
    MpegTsDecoder(const Options& options,
        const std::vector<std::shared_ptr<FrameProcessor>>& frameProcessors);
    virtual ~MpegTsDecoder();

    virtual int init() override;
    virtual void term() override;

protected:
    const std::string mH264Url;

private:
    int mFFmpegLogLevel;
    int mInputWidth;
    int mInputHeight;

    AVFormatContext *mInputContext;
    int mVideoStreamNum;
    int mDataStreamNum;
    AVCodec *mCodec;
    AVCodecContext *mCodecContext;
    AVFrame *mInputFrame, *mOutputFrame;
    struct SwsContext *mSwsContext;
    AVPacket mPkt;
    uint64_t mMetaDataBytes, mMetaDataSize;
    uint8_t* mMetaData;
    size_t mMetaDataBufferSize;


    std::thread *mPollThread;
    bool mPoll;

    void pollThread();
    int processDataFrame();
    int processVideoFrame();

    static const AVPixelFormat YUYV_PIXEL_FORMAT = AV_PIX_FMT_YUYV422;
    static const AVPixelFormat H264_PIXEL_FORMAT = AV_PIX_FMT_YUV420P;
};

