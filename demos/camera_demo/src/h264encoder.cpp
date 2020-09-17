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

#include "h264encoder.hpp"

extern "C" {
    #include <libavutil/opt.h>
}

H264Encoder::H264Encoder(const Options& options) :
    FrameProcessor((options.mVideoOutputType == H264) ? H264 : YUYV,
        options.mImageWidth, options.mImageHeight),
    mH264Url(options.mH264EncoderUrl),
    mFFmpegLogLevel(options.mFFmpegLogLevel),
    mFrameRateNumerator(options.mFrameRateNumerator),
    mFrameRateDenominator(options.mFrameRateDenominator),
    mCodec(nullptr),
    mCodecContext(nullptr),
    mInputFrame(nullptr),
    mOutputFrame(nullptr),
    mSwsContext(nullptr),
    mOutputContext(nullptr)
{

}

H264Encoder::~H264Encoder()
{
    term();
}

int H264Encoder::init()
{
    int rv;

    if ((mVideoType != YUYV) && (mVideoType != H264)) {
        std::cout << "h264 streamer requires yuyv or h264 input frames" << std::endl;
        return 1;
    }

    av_log_set_level(mFFmpegLogLevel);
    avcodec_register_all();
    av_register_all();
    avformat_network_init();

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
    av_opt_set(mCodecContext->priv_data, "preset", "fast", 0);
    av_opt_set(mCodecContext->priv_data, "tune", "zerolatency", 0);
    // bit rate suggestions https://support.google.com/youtube/answer/1722171?hl=en
    mCodecContext->bit_rate = 1000000;
    mCodecContext->width = mImageWidth;
    mCodecContext->height = mImageHeight;
    /* frames per second (inverse) */
    mCodecContext->time_base.num = mFrameRateNumerator;
    mCodecContext->time_base.den = mFrameRateDenominator;
    mCodecContext->gop_size = 10;
    mCodecContext->pix_fmt = H264_PIXEL_FORMAT;

    if (avcodec_open2(mCodecContext, mCodec, NULL) < 0) {
        std::cout << "unable to open h.264 codec" << std::endl;
        return 1;
    }

    AVOutputFormat* mpegFormat = av_guess_format("mpegts", NULL, NULL);
    if (mpegFormat == nullptr) {
        std::cout << "unable to locate mpeg-ts output format" << std::endl;
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

    mInputFrame->format = YUYV_PIXEL_FORMAT;
    mInputFrame->width  = mImageWidth;
    mInputFrame->height = mImageHeight;

    mOutputFrame->format = H264_PIXEL_FORMAT;
    mOutputFrame->width  = mImageWidth;
    mOutputFrame->height = mImageHeight;

    rv = av_frame_get_buffer(mInputFrame, 32);
    if (rv < 0) {
        std::cout << "unable to allocate input image buffer" << std::endl;
        return 1;
    }

    rv = av_frame_get_buffer(mOutputFrame, 32);
    if (rv < 0) {
        std::cout << "unable to allocate output image buffer" << std::endl;
        return 1;
    }

    mSwsContext = sws_getContext(mImageWidth, mImageHeight, YUYV_PIXEL_FORMAT,
        mImageWidth, mImageHeight, H264_PIXEL_FORMAT,
        0, nullptr, nullptr, nullptr);

    if (mSwsContext == nullptr) {
        std::cout << "unable to allocate SwsContext" << std::endl;
        return 1;
    }

    rv = avformat_alloc_output_context2(&mOutputContext, mpegFormat,
        mpegFormat->name, mH264Url.c_str());
    if (rv < 0) {
        std::cout << "unable to allocate output context" << std::endl;
        return 1;
    }

    rv = avio_open(&mOutputContext->pb, mOutputContext->filename, AVIO_FLAG_WRITE);
    if (rv < 0) {
        std::cout << "unable to open avio context" << std::endl;
        return 1;
    }
    struct AVStream* stream = avformat_new_stream(mOutputContext, mCodec);

    stream->codecpar->bit_rate = mCodecContext->bit_rate;
    stream->codecpar->width = mCodecContext->width;
    stream->codecpar->height = mCodecContext->height;
    stream->codecpar->codec_id = AV_CODEC_ID_H264;
    stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    stream->time_base.num = mCodecContext->time_base.num;
    stream->time_base.den = mCodecContext->time_base.den;

    rv = avformat_write_header(mOutputContext, NULL);
    if (rv < 0) {
        std::cout << "unable to write header" << std::endl;
        return 1;
    }

    return 0;
}

void H264Encoder::term()
{
    if (mOutputContext != nullptr) {
        av_write_trailer(mOutputContext);
        avio_close(mOutputContext->pb);
        avformat_free_context(mOutputContext);
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
    avformat_network_deinit();
}

int H264Encoder::processYUYV(FrameBuffer data, size_t length)
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
    while (rv == 0) {
        rv = avcodec_receive_packet(mCodecContext, &mPkt);
        if ((rv == AVERROR(EAGAIN)) || (rv == AVERROR_EOF)) {
            break;
        } else if (rv < 0) {
            std::cout << "avcodec_receive_packet error " << rv << std::endl;
            return rv;
        }
        rv = av_interleaved_write_frame(mOutputContext, &mPkt);
        if (rv) {
            std::cout << "av_interleaved_write_frame error " << rv << std::endl;
        }
        av_packet_unref(&mPkt);
    }

    return 0;
}

int H264Encoder::processH264(FrameBuffer data, size_t length)
{
    int rv;

    rv = av_new_packet(&mPkt, length);
    if (rv) {
        std::cout << "av_new_packet error " << rv << std::endl;
        return rv;
    }

    memcpy(mPkt.data, data, length);
    mPkt.pts = mIndex;
    mPkt.dts = mIndex;

    rv = av_interleaved_write_frame(mOutputContext, &mPkt);
    if (rv) {
        std::cout << "av_interleaved_write_frame error " << rv << std::endl;
    }

    av_packet_unref(&mPkt);
    return rv;
}

int H264Encoder::process(FrameBuffer data, size_t length)
{
    switch (mVideoType) {
        case YUYV:
            return processYUYV(data, length);
        case H264:
            return processH264(data, length);
        default:
            std::cout << "Unknown video type " << mVideoType << std::endl;
            return 1;
    }
}

