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

#include <cerrno>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>

#include <stdio.h>

#include "h264streamer.hpp"

extern "C" {
    #include <libavutil/imgutils.h>
}

H264Streamer::H264Streamer(const Options& options) :
    FrameProcessor(options.mVideoType, options.mImageWidth, options.mImageHeight),
    mCodec(nullptr),
    mCodecContext(nullptr),
    mInputFrame(nullptr),
    mOutputFrame(nullptr),
    mTempFile(nullptr)
{

}

H264Streamer::~H264Streamer()
{
    term();
}

int H264Streamer::init()
{
    int rv;

    avcodec_register_all();

    if (mVideoType != YUYV) {
        std::cout << "h264 streamer requires yuyv or h264 input frames" << std::endl;
        return 1;
    }

    mCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (mCodec == nullptr) {
        std::cout << "h.264 codec not found" << std::endl;
        return 1;
    }
    mCodecContext = avcodec_alloc_context3(mCodec);
    if (mCodecContext == nullptr) {
        std::cout << "unable to allocate h.264 codec context" << std::endl;
        return 1;
    }
    // assuming SDR 480p
    // https://support.google.com/youtube/answer/1722171?hl=en
    mCodecContext->bit_rate = 5000000;
    mCodecContext->width = mImageWidth;
    mCodecContext->height = mImageHeight;
    /* frames per second */
    mCodecContext->time_base.num = 1;
    mCodecContext->time_base.den = 25;
    mCodecContext->pix_fmt = AV_PIX_FMT_YUV422P;

    if (avcodec_open2(mCodecContext, mCodec, NULL) < 0) {
        std::cout << "unable to open h.264 codec" << std::endl;
        return 1;
    }

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

    mInputFrame->format = AV_PIX_FMT_YUYV422;
    mInputFrame->width  = mImageWidth;
    mInputFrame->height = mImageHeight;

    mOutputFrame->format = AV_PIX_FMT_YUV422P;
    mOutputFrame->width  = mImageWidth;
    mOutputFrame->height = mImageHeight;

    rv = av_image_alloc(mInputFrame->data, mInputFrame->linesize,
        mImageWidth, mImageHeight,
        AV_PIX_FMT_YUYV422, 32);
    if (rv < 0) {
        std::cout << "unable to allocate input image buffer" << std::endl;
        return 1;
    }

    rv = av_image_alloc(mOutputFrame->data, mOutputFrame->linesize,
        mImageWidth, mImageHeight,
        AV_PIX_FMT_YUV422P, 32);
    if (rv < 0) {
        std::cout << "unable to allocate output image buffer" << std::endl;
        return 1;
    }

    mSwsContext = sws_getContext(mImageWidth, mImageHeight, AV_PIX_FMT_YUYV422,
        mImageWidth, mImageHeight, AV_PIX_FMT_YUV422P,
        0, nullptr, nullptr, nullptr);
    
    if (mSwsContext == nullptr) {
        std::cout << "unable to allocate SwsContext" << std::endl;
        return 1;
    }

    mTempFile = fopen("/tmp/foobar.h264", "wb");

    return 0;
}

void H264Streamer::term()
{
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };

    if (mTempFile != nullptr) {
        fwrite(endcode, 1, sizeof(endcode), mTempFile);
        fclose(mTempFile);
    }
    if (mSwsContext != nullptr) {
        sws_freeContext(mSwsContext);
    }
    if (mOutputFrame != nullptr) {
        av_freep(&mOutputFrame->data[0]);
        av_frame_free(&mOutputFrame);
    }
    if (mInputFrame != nullptr) {
        av_freep(&mInputFrame->data[0]);
        av_frame_free(&mInputFrame);
    }
    if (mCodecContext != nullptr) {
        avcodec_close(mCodecContext);
        avcodec_free_context(&mCodecContext);
    }
}

unsigned char* H264Streamer::getFrame(unsigned index, VideoType videoType) {
    (void) index;
    (void) videoType;
    return nullptr;
}


int H264Streamer::process(FrameBuffer data, size_t length)
{
    int rv;

    av_init_packet(&mPkt);
    mPkt.data = NULL;    // packet data will be allocated by the encoder
    mPkt.size = 0;

    memcpy(mInputFrame->data[0], data, length);

    rv = sws_scale(mSwsContext, mInputFrame->data, mInputFrame->linesize,
        0, mImageHeight, mOutputFrame->data, mOutputFrame->linesize);

    if (rv != ((int) mImageHeight)) {
        std::cout << "sws_scale error " << rv << std::endl;
        return 1;
    }

    mOutputFrame->pts = mIndex;

    rv = avcodec_send_frame(mCodecContext, mOutputFrame);
    if (rv) {
        std::cout << "avcodec_send_frame error " << rv << std::endl;
        return rv;
    }
    while (1) {
        rv = avcodec_receive_packet(mCodecContext, &mPkt);
        if ((rv == AVERROR(EAGAIN)) || (rv == AVERROR_EOF)) {
            break;
        } else if (rv < 0) {
            std::cout << "avcodec_receive_packet error " << rv << std::endl;
            return rv;
        }
        fwrite(mPkt.data, 1, mPkt.size, mTempFile);
        av_packet_unref(&mPkt);
    }

    return 0;
}

