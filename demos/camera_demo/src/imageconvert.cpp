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

#include "imageconvert.hpp"

#include <climits>
#include <iostream>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <jpeglib.h>

ImageConvert::ImageConvert(unsigned width, unsigned height) :
    mImageWidth(width), mImageHeight(height) {

    mTempJpegBuffer = (unsigned char*) calloc(mImageWidth * mImageHeight * 3, 1);
    mTempJpegBufferRow = (unsigned char*) calloc(mImageWidth * 3, 1);
    mRGBXBuffer = (unsigned char*) calloc(mImageWidth * mImageHeight * 4, 1);
}

ImageConvert::~ImageConvert() {
    free(mTempJpegBuffer);
    free(mTempJpegBufferRow);
    free(mRGBXBuffer);
}

unsigned char* ImageConvert::getBuffer(VideoType videoType) const {
    if (videoType == RGBX) {
        return mRGBXBuffer;
    } else {
        return nullptr;
    }
}

size_t ImageConvert::expectedBytes(unsigned width, unsigned height, VideoType videoType) {
    switch (videoType) {
        case JPEG:
            return 0;
        case YUYV:
            return width * height * 2;
        case RGBX:
            return width * height * 4;
        default:
            return 0;
    }
}

int ImageConvert::convertJpegToRGBX(FrameBuffer src, size_t srcLength, unsigned char* dst) const {
    int i, width, depth;
    unsigned x, y, z, k;

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    JSAMPROW row_pointer[1];

    unsigned long location = 0;

    cinfo.err = jpeg_std_error(&jerr);

    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, src, srcLength);
    jpeg_read_header(&cinfo, 1);
    cinfo.scale_num = 1;
    cinfo.scale_denom = 1;

    jpeg_start_decompress(&cinfo);
    width = cinfo.output_width;
    depth = cinfo.num_components; //should always be 3
    if ((cinfo.output_width != mImageWidth) || (cinfo.output_height != mImageHeight)) {
        std::cout << "Expected " << mImageWidth << " x " << mImageHeight << " resolution"
        << " and received " << cinfo.output_width << " x " << cinfo.output_height << std::endl;
        return -1;
    }

    row_pointer[0] = mTempJpegBufferRow;

    while(cinfo.output_scanline < cinfo.output_height) {
	    jpeg_read_scanlines(&cinfo, row_pointer, 1);
	    for(i = 0; i < (width * depth); i++) {
	        mTempJpegBuffer[location++] = row_pointer[0][i];
        }
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    for(z = k = y = 0; y < mImageHeight; y++) {
	    for(x = 0; x < mImageWidth; x++) {
            // for 24 bit depth, organization BGRX
            dst[k+0]=mTempJpegBuffer[z+2];
            dst[k+1]=mTempJpegBuffer[z+1];
            dst[k+2]=mTempJpegBuffer[z+0];
            k+=4; z+=3;
        }
    }
    return 0;
}

static inline unsigned char clamp(int input) {
    if (input > UCHAR_MAX) {
        return UCHAR_MAX;
    } else if (input < 0) {
        return 0;
    } else {
        return input;
    }
}

int ImageConvert::convertYUYVToRGBX(FrameBuffer srcBuffer, unsigned char* dstBuffer) const {
    int c, d, e;
    size_t length = mImageWidth * mImageHeight * 2;

    for (size_t src = 0, dst = 0; src < length; src += 4, dst += 8) {
        d = (int) srcBuffer[src + 1] - 128;    // d = u - 128;
        e = (int) srcBuffer[src + 3] - 128;    // e = v - 128;
        // c = y’ - 16 (for first pixel)
        c = 298 * ((int) srcBuffer[src] - 16);
        // B - Blue
        dstBuffer[dst] = clamp((c + 516 * d + 128) >> 8);
        // G -Green
        dstBuffer[dst + 1] = clamp((c - 100 * d - 208 * e + 128) >> 8);
        // R - Red
        dstBuffer[dst + 2] = clamp((c + 409 * e + 128) >> 8);

        // c = y’ - 16 (for second pixel)
        c = 298 * ((int ) srcBuffer[src + 2] - 16);
        // B - Blue
        dstBuffer[dst + 4] = clamp((c + 516 * d + 128) >> 8);
        // G -Green
        dstBuffer[dst + 5] = clamp((c - 100 * d - 208 * e + 128) >> 8);
        // R - Red
        dstBuffer[dst + 6] = clamp((c + 409 * e + 128) >> 8);
    }
    return 0;
}

int ImageConvert::convert(FrameBuffer src, size_t srcLength, VideoType srcType, unsigned char* dst, VideoType dstType) const {
    switch (dstType) {
        case RGBX:
            switch (srcType) {
                case JPEG:
                    return convertJpegToRGBX(src, srcLength, dst);
                case YUYV:
                    return convertYUYVToRGBX(src, dst);
                default:
                    return -1;
            }
        default:
            return -1;
    }
}
