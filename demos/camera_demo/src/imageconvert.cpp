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
        mYUYVIndex(0), mBGRXIndex(0) {
    mYUYVBuffer = (unsigned char*) calloc(mImageWidth * mImageHeight * 2, 1);
    mBGRXBuffer = (unsigned char*) calloc(mImageWidth * mImageHeight * 4, 1);
    mRGBBuffer = (unsigned char*) calloc(mImageWidth * mImageHeight * 3, 1);
    mRGBBufferRow = (unsigned char*) calloc(mImageWidth * 3, 1);
}

ImageConvert::~ImageConvert() {
    free(mYUYVBuffer);
    free(mBGRXBuffer);
    free(mRGBBuffer);
    free(mRGBBufferRow);
}

#define CLAMP(X) ( (X) > 255 ? 255 : (X) < 0 ? 0 : X)

int ImageConvert::convert(FrameBuffer src, size_t srcLength, VideoType srcType,
        VideoType dstType, unsigned index) {
    int rv = -1;

    if (srcType == dstType) {
        std::cout << "video source type and destination type must be different" << std::endl;
        return 1;
    }

    if ((dstType != VIDEO_YUYV) && (dstType != VIDEO_BGRX)) {
        std::cout << "video destination type must be YUYV or BGRX" << std::endl;
        return 1;
    }

    if ((srcType != VIDEO_JPEG) && (srcType != VIDEO_YUYV)) {
        std::cout << "video source type must be JPEG or YUYV" << std::endl;
        return 1;
    }

    switch (dstType) {
        case VIDEO_YUYV:
            switch (srcType) {
                case VIDEO_JPEG:
                    rv = jpegToYUYV(src, srcLength, index);
                    break;
                default:
                    break;
            }
            break;
        case VIDEO_BGRX:
            switch (srcType) {
                case VIDEO_YUYV:
                    rv = yuyvToBGRX(src, srcLength, index);
                    break;
                case VIDEO_JPEG:
                    rv = jpegToBGRX(src, srcLength, index);
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
        case VIDEO_YUYV:
            if (index != mYUYVIndex) return nullptr;
            *length = mImageWidth * mImageHeight * 2;
            return mYUYVBuffer;
        case VIDEO_BGRX:
            if (index != mBGRXIndex) return nullptr;
            *length = mImageWidth * mImageHeight * 4;
            return mBGRXBuffer;
        default:
            return nullptr;
    }
}

int ImageConvert::jpegToBGRX(FrameBuffer src, size_t length, unsigned index) {
    int rv;
    if (mBGRXIndex == index) {
        return 0;
    }
    rv = jpegToRGB(src, length);
    if (rv) {
        return rv;
    }
    rv = rgbToBGRX();
    if (rv) {
        return rv;
    }
    mBGRXIndex = index;
    return 0;
}

int ImageConvert::jpegToYUYV(FrameBuffer src, size_t length, unsigned index) {
    int rv;
    if (mYUYVIndex == index) {
        return 0;
    }
    rv = jpegToRGB(src, length);
    if (rv) {
        return rv;
    }
    rv = rgbToYUYV();
    if (rv) {
        return rv;
    }
    mYUYVIndex = index;
    return 0;
}

int ImageConvert::jpegToRGB(FrameBuffer src, size_t srcLength) {
#ifndef JPEG_PRESENT
    std::cout << "JPEG library support not compiled" << std::endl;
    return 1;
#else
    int i, width, depth;

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    JSAMPROW row_pointer[1];

    unsigned long location = 0;

    cinfo.err = jpeg_std_error(&jerr);

    jpeg_create_decompress(&cinfo);
    // Cast src to a (unsigned char*) for compatibility with libjpeg-turbo 1.2.1 and earlier.
    // Needed for Centos 7 compatibility.
    jpeg_mem_src(&cinfo, (unsigned char*) src, srcLength);
    jpeg_read_header(&cinfo, 1);
    cinfo.scale_num = 1;
    cinfo.scale_denom = 1;

    jpeg_start_decompress(&cinfo);
    width = cinfo.output_width;
    depth = cinfo.num_components; //should always be 3
    if (depth != 3) {
        std::cout << "Expected 3 components and received " << depth << " components" << std::endl;
        return -1;
    }
    if ((cinfo.output_width != mImageWidth) || (cinfo.output_height != mImageHeight)) {
        std::cout << "Expected " << mImageWidth << " x " << mImageHeight << " resolution"
        << " and received " << cinfo.output_width << " x " << cinfo.output_height << std::endl;
        return -1;
    }

    row_pointer[0] = mRGBBufferRow;

    while(cinfo.output_scanline < cinfo.output_height) {
	    jpeg_read_scanlines(&cinfo, row_pointer, 1);
	    for(i = 0; i < (width * depth); i++) {
	        mRGBBuffer[location++] = row_pointer[0][i];
        }
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    return 0;
#endif
}

int ImageConvert::rgbToBGRX() {
    unsigned x, y, z, k;

    for(z = k = y = 0; y < mImageHeight; y++) {
	    for(x = 0; x < mImageWidth; x++) {
            // for 24 bit depth, organization BGRX
            mBGRXBuffer[k+0]=mRGBBuffer[z+2];
            mBGRXBuffer[k+1]=mRGBBuffer[z+1];
            mBGRXBuffer[k+2]=mRGBBuffer[z+0];
            k+=4; z+=3;
        }
    }
    return 0;
}

#define RGB2Y(R, G, B) CLAMP(( (  66 * (R) + 129 * (G) +  25 * (B) + 128) >> 8) +  16)
#define RGB2U(R, G, B) CLAMP(( ( -38 * (R) -  74 * (G) + 112 * (B) + 128) >> 8) + 128)
#define RGB2V(R, G, B) CLAMP(( ( 112 * (R) -  94 * (G) -  18 * (B) + 128) >> 8) + 128)


int ImageConvert::rgbToYUYV() {
    size_t length = mImageWidth * mImageHeight * 3;

    for (size_t src = 0, dst = 0; src < length; src += 6, dst += 4) {
        int y1 = RGB2Y(mRGBBuffer[src + 0], mRGBBuffer[src + 1], mRGBBuffer[src + 2]);
        int u1 = RGB2U(mRGBBuffer[src + 0], mRGBBuffer[src + 1], mRGBBuffer[src + 2]);
        int v1 = RGB2V(mRGBBuffer[src + 0], mRGBBuffer[src + 1], mRGBBuffer[src + 2]);

        int y2 = RGB2Y(mRGBBuffer[src + 3], mRGBBuffer[src + 4], mRGBBuffer[src + 5]);
        int u2 = RGB2U(mRGBBuffer[src + 3], mRGBBuffer[src + 4], mRGBBuffer[src + 5]);
        int v2 = RGB2V(mRGBBuffer[src + 3], mRGBBuffer[src + 4], mRGBBuffer[src + 5]);

        int u = (u1 + u2) / 2;
        int v = (v1 + v2) / 2;

        mYUYVBuffer[dst] = y1;
        mYUYVBuffer[dst + 1] = u;
        mYUYVBuffer[dst + 2] = y2;
        mYUYVBuffer[dst + 3] = v;
    }
    return 0;
}

int ImageConvert::yuyvToBGRX(FrameBuffer srcBuffer, size_t length, unsigned index) {
    int c, d, e;

    if (mBGRXIndex == index) {
        return 0;
    }

    for (size_t src = 0, dst = 0; src < length; src += 4, dst += 8) {
        d = (int) srcBuffer[src + 1] - 128;    // d = u - 128;
        e = (int) srcBuffer[src + 3] - 128;    // e = v - 128;
        // c = y’ - 16 (for first pixel)
        c = 298 * ((int) srcBuffer[src] - 16);
        // B - Blue
        mBGRXBuffer[dst] = CLAMP((c + 516 * d + 128) >> 8);
        // G -Green
        mBGRXBuffer[dst + 1] = CLAMP((c - 100 * d - 208 * e + 128) >> 8);
        // R - Red
        mBGRXBuffer[dst + 2] = CLAMP((c + 409 * e + 128) >> 8);

        // c = y’ - 16 (for second pixel)
        c = 298 * ((int ) srcBuffer[src + 2] - 16);
        // B - Blue
        mBGRXBuffer[dst + 4] = CLAMP((c + 516 * d + 128) >> 8);
        // G -Green
        mBGRXBuffer[dst + 5] = CLAMP((c - 100 * d - 208 * e + 128) >> 8);
        // R - Red
        mBGRXBuffer[dst + 6] = CLAMP((c + 409 * e + 128) >> 8);
    }
    mBGRXIndex = index;
    return 0;
}
