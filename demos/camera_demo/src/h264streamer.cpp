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
    #include <libavutil/opt.h>
}

H264Streamer::H264Streamer(const Options& options) :
    FrameProcessor(options.mVideoType, options.mImageWidth, options.mImageHeight),
    mH264Url(options.mH264Url),
    mCodec(nullptr),
    mCodecContext(nullptr),
    mInputFrame(nullptr),
    mOutputFrame(nullptr),
    mSwsContext(nullptr),
    mOutputContext(nullptr),
    mSnapshotTime(0),
    mSnapshotIndex(0)
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
    av_register_all();
    avformat_network_init();

    if ((mVideoType != YUYV) && (mVideoType != H264)) {
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
    av_opt_set(mCodecContext->priv_data, "preset", "fast", 0);
    av_opt_set(mCodecContext->priv_data, "tune", "zerolatency", 0);
    // bit rate suggestions https://support.google.com/youtube/answer/1722171?hl=en
    mCodecContext->bit_rate = 1000000;
    mCodecContext->width = mImageWidth;
    mCodecContext->height = mImageHeight;
    /* frames per second */
    mCodecContext->time_base.num = 1;
    mCodecContext->time_base.den = FRAMES_PER_SECOND;
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

    rv = av_image_alloc(mInputFrame->data, mInputFrame->linesize,
        mImageWidth, mImageHeight,
        YUYV_PIXEL_FORMAT, 32);
    if (rv < 0) {
        std::cout << "unable to allocate input image buffer" << std::endl;
        return 1;
    }

    rv = av_image_alloc(mOutputFrame->data, mOutputFrame->linesize,
        mImageWidth, mImageHeight,
        H264_PIXEL_FORMAT, 32);
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

    mSnapshotTime = time(NULL);

    return 0;
}

void H264Streamer::term()
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
    avformat_network_deinit();
}

unsigned char* H264Streamer::getFrame(unsigned index, VideoType videoType) {
    (void) index;
    (void) videoType;
    return nullptr;
}


int H264Streamer::process(FrameBuffer data, size_t length)
{
    int rv;
    time_t currentTime;

    // Drop unnecessary frames
    currentTime = time(NULL);
    if (currentTime != mSnapshotTime) {
        mSnapshotTime = currentTime;
        mSnapshotIndex = mIndex;
    } else if ((mIndex - mSnapshotIndex) > FRAMES_PER_SECOND) {
        return 0;
    }

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

