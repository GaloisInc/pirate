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
        const std::vector<std::shared_ptr<FrameProcessor>>& frameProcessors) :
    mFrameProcessors(frameProcessors),
    mInputWidth(1920),
    mInputHeight(1080),
    mOutputWidth(options.mImageWidth),
    mOutputHeight(options.mImageHeight),
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
    avcodec_register_all();
    av_register_all();
    avformat_network_init();

    mInputContext = avformat_alloc_context();
    
    if (avformat_open_input(&mInputContext, "udp://localhost:15004", NULL, NULL) < 0) {
        std::cout << "unable to open url " << "udp://localhost:15004" << std::endl;
        term();
        return 1;
    }

    if (mInputContext->nb_streams == 0) {
        std::cout << "no streams found at url " << "udp://localhost:15004" << std::endl;        
        term();
        return 1;
    }

    mVideoStreamNum = av_find_best_stream(mInputContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    mDataStreamNum  = av_find_best_stream(mInputContext, AVMEDIA_TYPE_DATA,  -1, -1, NULL, 0);

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
    mOutputFrame->width  = mOutputWidth;
    mOutputFrame->height = mOutputHeight;

    mSwsContext = sws_getContext(mInputWidth, mInputHeight, H264_PIXEL_FORMAT,
        mOutputWidth, mOutputHeight, YUYV_PIXEL_FORMAT,
        0, nullptr, nullptr, nullptr);

    if (mSwsContext == nullptr) {
        std::cout << "unable to allocate SwsContext" << std::endl;
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
}

void H264Decoder::pollThread()
{
    int index, rv;
    unsigned frameNumber = 0;

    while (mPoll)
    {
        std::cout << "doing something" << std::endl;
        rv = av_read_frame(mInputContext, &mPkt);
        if (rv) {
            std::cout << "av_read_frame error " << rv << std::endl;
            return;
        }

        index = mPkt.stream_index;

        if (index == mDataStreamNum) {
            // TODO: process data stream
        } else if (index == mVideoStreamNum) {
            rv = 0;

            while (rv == 0) {
                rv = avcodec_send_packet(mCodecContext, &mPkt);
                if (rv < 0) {
                    std::cout << "avcodec_send_packet error " << rv << std::endl;
                    return;
                }

                rv = avcodec_receive_frame(mCodecContext, mInputFrame);
                if ((rv == AVERROR(EAGAIN)) || (rv == AVERROR_EOF)) {
                    break;
                } else if (rv < 0) {
                    std::cout << "avcodec_receive_packet error " << rv << std::endl;
                    return;
                }
                std::cout << "received a frame" << std::endl;

                rv = sws_scale(mSwsContext, mInputFrame->data, mInputFrame->linesize,
                    0, mInputHeight, mOutputFrame->data, mOutputFrame->linesize);

                if (rv != ((int) mOutputHeight)) {
                    std::cout << "sws_scale error " << rv << std::endl;
                    return;
                }

                frameNumber++;
                for (size_t i = 0; i < mFrameProcessors.size(); i++) {
                    auto current = mFrameProcessors[i];
                    current->processFrame(mOutputFrame->data[0], mOutputHeight * mOutputWidth * 2);
                }
            }
        }
        av_packet_unref(&mPkt);
    }
}
