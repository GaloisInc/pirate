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

#ifdef JPEG_PRESENT
#include <jpeglib.h>
#endif

ImageConvert::ImageConvert(unsigned width, unsigned height) :
    mImageWidth(width), mImageHeight(height),
    mYUYVIndex(0), mRGBXIndex(0) {

    mYUYVBuffer = (unsigned char*) calloc(mImageWidth * mImageHeight * 2, 1);
    mRGBXBuffer = (unsigned char*) calloc(mImageWidth * mImageHeight * 4, 1);

    mTempJpegBuffer = (unsigned char*) calloc(mImageWidth * mImageHeight * 3, 1);
    mTempJpegBufferRow = (unsigned char*) calloc(mImageWidth * 3, 1);

}

ImageConvert::~ImageConvert() {
    free(mYUYVBuffer);
    free(mRGBXBuffer);

    free(mTempJpegBuffer);
    free(mTempJpegBufferRow);

}

int ImageConvert::convert(FrameBuffer src, size_t srcLength, VideoType srcType,
        VideoType dstType, unsigned index) {
    int rv = -1;

    if (srcType == dstType) {
        std::cout << "video source type and destination type must be different" << std::endl;
        return 1;
    }

    if ((dstType != YUYV) && (dstType != RGBX)) {
        std::cout << "video destination type must be YUYV or RGBX" << std::endl;
        return 1;
    }

    if ((srcType != JPEG) && (srcType != YUYV)) {
        std::cout << "video source type must be JPEG or YUYV" << std::endl;
        return 1;
    }

    switch (dstType) {
        case YUYV:
            switch (srcType) {
                case JPEG:
                    rv = jpegToYUYV(src, srcLength, index);
                    break;
                default:
                    break;
            }
            break;
        case RGBX:
            switch (srcType) {
                case YUYV:
                    rv = yuyvToRGBX(src, srcLength, index);
                    break;
                case JPEG:
                    rv = jpegToRGBX(src, srcLength, index);
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }

    return rv;
}

unsigned char* ImageConvert::getBuffer(VideoType videoType, unsigned index, size_t* length) const {
    switch (videoType) {
        case YUYV:
            if (index != mYUYVIndex) return nullptr;
            *length = mImageWidth * mImageHeight * 2;
            return mYUYVBuffer;
        case RGBX:
            if (index != mRGBXIndex) return nullptr;
            *length = mImageWidth * mImageHeight * 4;
            return mRGBXBuffer;
        default:
            return nullptr;
    }
}

int ImageConvert::jpegToRGBX(FrameBuffer src, size_t srcLength, unsigned index) {
#ifndef JPEG_PRESENT
    std::cout << "JPEG library support not compiled" << std::endl;
    return 1;
#else
    int i, width, depth;
    unsigned x, y, z, k;

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    JSAMPROW row_pointer[1];

    unsigned long location = 0;

    if (mRGBXIndex == index) {
        return 0;
    }

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
            mRGBXBuffer[k+0]=mTempJpegBuffer[z+2];
            mRGBXBuffer[k+1]=mTempJpegBuffer[z+1];
            mRGBXBuffer[k+2]=mTempJpegBuffer[z+0];
            k+=4; z+=3;
        }
    }
    mRGBXIndex = index;
    return 0;
#endif
}

int ImageConvert::jpegToYUYV(FrameBuffer src, size_t srcLength, unsigned index) {
    int rv;

    rv = jpegToRGBX(src, srcLength, index);
    if (rv) {
        return rv;
    }
    rv = rgbxToYUYV(mRGBXBuffer, mImageWidth * mImageHeight * 4, index);
    if (rv) {
        return rv;
    }
    return 0;
}

#define CLAMP(X) ( (X) > 255 ? 255 : (X) < 0 ? 0 : X)

int ImageConvert::yuyvToRGBX(FrameBuffer srcBuffer, size_t length, unsigned index) {
    int c, d, e;

    if (mRGBXIndex == index) {
        return 0;
    }

    for (size_t src = 0, dst = 0; src < length; src += 4, dst += 8) {
        d = (int) srcBuffer[src + 1] - 128;    // d = u - 128;
        e = (int) srcBuffer[src + 3] - 128;    // e = v - 128;
        // c = y’ - 16 (for first pixel)
        c = 298 * ((int) srcBuffer[src] - 16);
        // B - Blue
        mRGBXBuffer[dst] = CLAMP((c + 516 * d + 128) >> 8);
        // G -Green
        mRGBXBuffer[dst + 1] = CLAMP((c - 100 * d - 208 * e + 128) >> 8);
        // R - Red
        mRGBXBuffer[dst + 2] = CLAMP((c + 409 * e + 128) >> 8);

        // c = y’ - 16 (for second pixel)
        c = 298 * ((int ) srcBuffer[src + 2] - 16);
        // B - Blue
        mRGBXBuffer[dst + 4] = CLAMP((c + 516 * d + 128) >> 8);
        // G -Green
        mRGBXBuffer[dst + 5] = CLAMP((c - 100 * d - 208 * e + 128) >> 8);
        // R - Red
        mRGBXBuffer[dst + 6] = CLAMP((c + 409 * e + 128) >> 8);
    }
    mRGBXIndex = index;
    return 0;
}

#define RGB2Y(R, G, B) CLAMP(( (  66 * (R) + 129 * (G) +  25 * (B) + 128) >> 8) +  16)
#define RGB2U(R, G, B) CLAMP(( ( -38 * (R) -  74 * (G) + 112 * (B) + 128) >> 8) + 128)
#define RGB2V(R, G, B) CLAMP(( ( 112 * (R) -  94 * (G) -  18 * (B) + 128) >> 8) + 128)


int ImageConvert::rgbxToYUYV(FrameBuffer srcBuffer, size_t length, unsigned index) {
    if (mYUYVIndex == index) {
        return 0;
    }
    for (size_t src = 0, dst = 0; src < length; src += 8, dst += 4) {
        int y1 = RGB2Y(srcBuffer[src + 2], srcBuffer[src + 1], srcBuffer[src + 0]);
        int u1 = RGB2U(srcBuffer[src + 2], srcBuffer[src + 1], srcBuffer[src + 0]);
        int v1 = RGB2V(srcBuffer[src + 2], srcBuffer[src + 1], srcBuffer[src + 0]);

        int y2 = RGB2Y(srcBuffer[src + 6], srcBuffer[src + 5], srcBuffer[src + 4]);
        int u2 = RGB2U(srcBuffer[src + 6], srcBuffer[src + 5], srcBuffer[src + 4]);
        int v2 = RGB2V(srcBuffer[src + 6], srcBuffer[src + 5], srcBuffer[src + 4]);

        int u = (u1 + u2) / 2;
        int v = (v1 + v2) / 2;

        mYUYVBuffer[dst] = y1;
        mYUYVBuffer[dst + 1] = u;
        mYUYVBuffer[dst + 2] = y2;
        mYUYVBuffer[dst + 3] = v;
    }
    mYUYVIndex = index;
    return 0;
}
