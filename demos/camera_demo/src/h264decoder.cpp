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

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <string>
#include <cerrno>
#include <cstring>
#include <iostream>

#include "options.hpp"
#include "h264decoder.hpp"

H264Decoder::H264Decoder(const Options& options,
        const std::vector<std::shared_ptr<FrameProcessor>>& frameProcessors,
        const ImageConvert& imageConvert) :
    VideoSource(options, frameProcessors, imageConvert),
    mH264Url(options.mH264DecoderUrl),
    mInputWidth(0),
    mInputHeight(0),
    mInputContext(nullptr),
    mVideoStreamNum(-1),
    mDataStreamNum(-1),
    mCodec(nullptr),
    mCodecContext(nullptr),
    mInputFrame(nullptr),
    mOutputFrame(nullptr),
    mSwsContext(nullptr),
    mPollThread(nullptr),
    mPoll(false)
{
}

H264Decoder::~H264Decoder()
{
    term();
}

int H264Decoder::init()
{
    int rv;

    if (mH264Url.empty()) {
        std::cout << "decoder url must be specified on the command-line" << std::endl;
        return 1;
    }

    avcodec_register_all();
    av_register_all();
    avformat_network_init();

    mInputContext = avformat_alloc_context();
    
    if (avformat_open_input(&mInputContext, mH264Url.c_str(), NULL, NULL) < 0) {
        std::cout << "unable to open url " << mH264Url << std::endl;
        return 1;
    }

    if (mInputContext->nb_streams == 0) {
        std::cout << "no streams found at url " << mH264Url << std::endl;
        return 1;
    }

    mVideoStreamNum = av_find_best_stream(mInputContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    mDataStreamNum  = av_find_best_stream(mInputContext, AVMEDIA_TYPE_DATA,  -1, -1, NULL, 0);

    if (mVideoStreamNum < 0) {
        std::cout << "no video streams found at url " << mH264Url << std::endl;
        return 1;
    }

    av_read_play(mInputContext);

    mCodec = avcodec_find_decoder(mInputContext->streams[mVideoStreamNum]->codecpar->codec_id);
    mCodecContext = avcodec_alloc_context3(mCodec);

    // Open the newly allocated codec context
    avcodec_open2(mCodecContext, mCodec, NULL);

    mInputFrame = av_frame_alloc();
    if (mInputFrame == nullptr) {
        std::cout << "unable to allocate input frame" << std::endl;
        return 1;
    }
    mOutputFrame = av_frame_alloc();
    if (mOutputFrame == nullptr) {
        std::cout << "unable to allocate output frame" << std::endl;
        return 1;
    }

    mOutputFrame->format = YUYV_PIXEL_FORMAT;
    mOutputFrame->width  = mImageWidth;
    mOutputFrame->height = mImageHeight;

    rv = av_frame_get_buffer(mOutputFrame, 32);
    if (rv < 0) {
        std::cout << "unable to allocate output image buffer" << std::endl;
        return 1;
    }

    av_init_packet(&mPkt);
    mPkt.data = NULL;
    mPkt.size = 0;

    // Start the capture thread
    mPoll = true;
    mPollThread = new std::thread(&H264Decoder::pollThread, this);

    return 0;
}

void H264Decoder::term()
{
    if (mPoll)
    {
        mPoll = false;
        if (mPollThread != nullptr)
        {
            mPollThread->join();
            delete mPollThread;
            mPollThread = nullptr;
        }
    }
    if (mSwsContext != nullptr) {
        sws_freeContext(mSwsContext);
    }
    if (mOutputFrame != nullptr) {
        av_frame_free(&mOutputFrame);
    }
    if (mInputFrame != nullptr) {
        av_frame_free(&mInputFrame);
    }
    if (mCodecContext != nullptr) {
        avcodec_close(mCodecContext);
        avcodec_free_context(&mCodecContext);
    }
    if (mInputContext != nullptr) {
        avformat_close_input(&mInputContext);
        avformat_free_context(mInputContext);
    }
}

int H264Decoder::processVideoFrame() {
    int rv;

    rv = avcodec_send_packet(mCodecContext, &mPkt);
    if (rv < 0) {
        // skip invalid data
        if (rv == AVERROR_INVALIDDATA) {
            return 0;
        } else {
            std::cout << "avcodec_send_packet error " << rv << std::endl;
            return -1;
        }
    }

    rv = avcodec_receive_frame(mCodecContext, mInputFrame);
    if ((rv == AVERROR(EAGAIN)) || (rv == AVERROR_EOF)) {
        return 0;
    } else if (rv < 0) {
        std::cout << "avcodec_receive_packet error " << rv << std::endl;
        return -1;
    }

    if ((mInputWidth != mInputFrame->width) || (mInputHeight != mInputFrame->height)) {
        if (mSwsContext != nullptr) {
            sws_freeContext(mSwsContext);
        }

        mSwsContext = sws_getContext(mInputFrame->width, mInputFrame->height,
            H264_PIXEL_FORMAT, mImageWidth, mImageHeight,
            YUYV_PIXEL_FORMAT, 0, nullptr, nullptr, nullptr);

        if (mSwsContext == nullptr) {
            std::cout << "unable to allocate SwsContext" << std::endl;
            return -1;
        }

        mInputWidth  = mInputFrame->width;
        mInputHeight = mInputFrame->height;
    }

    rv = sws_scale(mSwsContext, mInputFrame->data, mInputFrame->linesize,
        0, mInputHeight, mOutputFrame->data, mOutputFrame->linesize);

    if (rv != ((int) mImageHeight)) {
        std::cout << "sws_scale error " << rv << std::endl;
        return -1;
    }

    rv = process(mOutputFrame->data[0], mImageWidth * mImageHeight * 2);
    if (rv) {
        return rv;
    }

    return 0;
}

void H264Decoder::pollThread()
{
    int index, rv;

    while (mPoll)
    {
        rv = av_read_frame(mInputContext, &mPkt);
        if (rv) {
            std::cout << "av_read_frame error " << rv << std::endl;
            return;
        }

        index = mPkt.stream_index;

        if (index == mDataStreamNum) {
            // TODO: process data stream
        } else if (index == mVideoStreamNum) {
            rv = processVideoFrame();
        }
        av_packet_unref(&mPkt);
        if (rv) {
            return;
        }
    }
}
