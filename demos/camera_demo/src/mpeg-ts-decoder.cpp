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
#include <unistd.h>

#include <string>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <iostream>

#include "options.hpp"
#include "mpeg-ts-decoder.hpp"

MpegTsDecoder::MpegTsDecoder(const Options& options,
        const std::vector<std::shared_ptr<FrameProcessor>>& frameProcessors) :
    VideoSource(options, frameProcessors),
    mH264Url(options.mH264DecoderUrl),
    mFFmpegLogLevel(options.mFFmpegLogLevel),
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
    mMetaDataBytes(0),
    mMetaDataSize(0),
    mMetaData(nullptr),
    mMetaDataBufferSize(0),
    mPollThread(nullptr),
    mPoll(false)
{
}

MpegTsDecoder::~MpegTsDecoder()
{
    term();
}

int MpegTsDecoder::init()
{
    int rv;
    AVDictionary* opts = NULL;

    if (mH264Url.empty()) {
        std::cout << "decoder url must be specified on the command-line" << std::endl;
        return 1;
    }

    rv = VideoSource::init();
    if (rv) {
        return rv;
    }

    av_log_set_level(mFFmpegLogLevel);
    avcodec_register_all();
    av_register_all();
    avformat_network_init();

    mInputContext = avformat_alloc_context();
    
    // udp protocol timeout is in microseconds
    av_dict_set(&opts, "timeout", "2000000", 0);
    if (avformat_open_input(&mInputContext, mH264Url.c_str(), NULL, &opts) < 0) {
        if (opts != NULL) av_dict_free(&opts);
        std::cout << "unable to open url " << mH264Url << std::endl;
        return 1;
    } else {
        // on success opts will be destroyed and replaced
        // with a dict containing options that were not found
        if (opts != NULL) av_dict_free(&opts);
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
    mOutputFrame->width  = mOutputWidth;
    mOutputFrame->height = mOutputHeight;

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
    mPollThread = new std::thread(&MpegTsDecoder::pollThread, this);

    return 0;
}

void MpegTsDecoder::term()
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
    if (mMetaData != nullptr) {
        free(mMetaData);
    }
}

#ifndef MIN
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#endif

int MpegTsDecoder::processDataFrame() {
    // If we have a full metadata packet in memory, zero out the size and index
    if (mMetaDataBytes == mMetaDataSize) {
        mMetaDataBytes = mMetaDataSize = 0;
    }

    // If we don't have any metadata buffered up yet and this packet is big enough for a US key and size
    if ((mMetaDataBytes == 0) && (mPkt.size > 17)) {
        // UAS LS universal key
        static const uint8_t KlvHeader[16] = {
            0x06, 0x0E, 0x2B, 0x34, 0x02, 0x0B, 0x01, 0x01,
            0x0E, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0x00
        };

        // Try finding the KLV header in this packet
        const uint8_t *pStart = (const uint8_t*) memmem(mPkt.data, mPkt.size, KlvHeader, 16);
        const uint8_t *pSize = pStart + 16;

        // If we found the header and the size tag is contained in this packet
        if ((pStart != 0) && ((pSize - mPkt.data) < mPkt.size)) {
            // Initialize the header size to US key + 1 size byte and zero KLV tag bytes
            uint64_t klvSize = 0, headerSize = 17;

            // If the size is a multi-byte BER-OID size
            if (pSize[0] & 0x80) {
                // Get the size of the size (up to )
                int bytes = pSize[0] & 0x07, i;

                // If the entire size field is contained in this packet
                if (&pSize[bytes] < &mPkt.data[mPkt.size]) {
                    // Build the size up from the individual bytes
                    for (i = 0; i < bytes; i++) {
                        klvSize = (klvSize << 8) | pSize[i + 1];
                    }
                }

                // Add the additional size bytes to the header size
                headerSize += bytes;
            }
            // Otherwise, just read the size byte straight out of byte 16
            else {
                klvSize = pSize[0];
            }

            // If we got a valid local set size
            if (klvSize > 0) {
                // Compute the maximum bytes to copy out of the packet
                size_t maxBytes = mPkt.size - (pStart - mPkt.data);
                size_t totalSize = headerSize + klvSize;
                size_t bytesToCopy = MIN(maxBytes, totalSize);

                // If our local buffer is too small for the incoming data
                if (mMetaDataBufferSize < totalSize) {
                    // Reallocate enough space and store the new buffer size
                    mMetaData = (uint8_t *) realloc(mMetaData, totalSize);
                    mMetaDataBufferSize = totalSize;
                }

                // Now copy the new data into the start of the local buffer
                memcpy(mMetaData, pStart, bytesToCopy);
                mMetaDataSize = totalSize;
                mMetaDataBytes = bytesToCopy;
            }
        }
    // Otherwise, if we're mid-packet
    } else if (mMetaDataBytes < mMetaDataSize) {
        // Figure out the number of bytes to copy out of this particular packet
        int bytesToCopy = MIN(((unsigned) mPkt.size), mMetaDataSize - mMetaDataBytes);

        // Copy into the local buffer in the right spot and increment the index
        memcpy(&mMetaData[mMetaDataBytes], mPkt.data, bytesToCopy);
        mMetaDataBytes += bytesToCopy;
    }

    if ((mMetaDataSize > 0) && (mMetaDataBytes == mMetaDataSize)) {
        return process(mMetaData, mMetaDataSize, MetaData);
    }

    return 0;
}

// If there are decoding errors in the video frame
// then drop the frame and do not stop the frame
// processing pipeline.
int MpegTsDecoder::processVideoFrame() {
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
            H264_PIXEL_FORMAT, mOutputWidth, mOutputHeight,
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

    if (rv != ((int) mOutputHeight)) {
        std::cout << "sws_scale error " << rv << std::endl;
        return -1;
    }

    rv = process(mOutputFrame->data[0], mOutputWidth * mOutputHeight * 2, VideoData);
    if (rv) {
        return rv;
    }

    return 0;
}

void MpegTsDecoder::pollThread()
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
            rv = processDataFrame();
        } else if (index == mVideoStreamNum) {
            rv = processVideoFrame();
        }
        av_packet_unref(&mPkt);
        if (rv) {
            return;
        }
    }
}
