#include <cerrno>
#include <climits>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>
#include "xwinframeprocessor.hpp"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <jpeglib.h>

int XWinFrameProcessor::xwinDisplayInitialize() {
    mDisplay = XOpenDisplay(nullptr);
    if (mDisplay == nullptr) {
        std::cout << "Failed to open X display" << std::endl;
        return -1;
    }
    int x_screen = DefaultScreen(mDisplay);
    mWindow = XCreateSimpleWindow(mDisplay,
        RootWindow(mDisplay, x_screen), 10, 10, mImageWidth, mImageHeight, 1,
        BlackPixel(mDisplay, x_screen), WhitePixel(mDisplay, x_screen));
    mContext = XCreateGC(mDisplay, mWindow, 0, &mContextVals);
    mImageBuffer = (unsigned char*) calloc(mImageWidth * mImageHeight * 4, 1);
    switch (mVideoType) {
        case JPEG:
            mTempImageBuffer = (unsigned char*) calloc(mImageWidth * mImageHeight * 3, 1);
            mTempImageBufferRow = (unsigned char*) calloc(mImageWidth * 3, 1);
            break;
        case YUYV:
            mTempImageBuffer = (unsigned char*) calloc(mImageWidth * mImageHeight * 2, 1);
            mTempImageBufferRow = nullptr;
            break;
        default:
            std::cout << "Unknown video type " << mVideoType << std::endl;
            return -1;
    }
    if (mMonochrome && (mVideoType != YUYV)) {
            std::cout << "Monochrome filter cannot be used with video type " << mVideoType << std::endl;
            return -1;
    }
    mImage = XCreateImage(mDisplay, CopyFromParent, 24, ZPixmap, 0, (char*) mImageBuffer,
        mImageWidth, mImageHeight, 32, 4 * mImageWidth);
    XMapWindow(mDisplay, mWindow);
    XSync(mDisplay, 0);
    return 0;
}


void XWinFrameProcessor::xwinDisplayTerminate() {
    if (mDisplay != nullptr) {
        XDestroyImage(mImage); // frees mImageBuffer
        XFreeGC(mDisplay, mContext);
        XCloseDisplay(mDisplay);
        free(mTempImageBuffer);
        if (mTempImageBufferRow != nullptr) {
            free(mTempImageBufferRow);
        }
    }
    mImage = nullptr;
    mDisplay = nullptr;
    mImageBuffer = nullptr;
    mTempImageBuffer = nullptr;
    mTempImageBufferRow = nullptr;
    mContext = nullptr;
}

int XWinFrameProcessor::convertJpeg(FrameBuffer buf, size_t len) {
    int i, width, depth;
    unsigned x, y, z, k;

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    JSAMPROW row_pointer[1];

    unsigned long location = 0;

    cinfo.err = jpeg_std_error(&jerr);

    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, buf, len);
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

    row_pointer[0] = mTempImageBufferRow;

    while(cinfo.output_scanline < cinfo.output_height) {
	    jpeg_read_scanlines(&cinfo, row_pointer, 1);
	    for(i = 0; i < (width * depth); i++) {
	        mTempImageBuffer[location++] = row_pointer[0][i];
        }
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    for(z = k = y = 0; y < mImageHeight; y++) {
	    for(x = 0; x < mImageWidth; x++) {
            // for 24 bit depth, organization BGRX
            mImageBuffer[k+0]=mTempImageBuffer[z+2];
            mImageBuffer[k+1]=mTempImageBuffer[z+1];
            mImageBuffer[k+2]=mTempImageBuffer[z+0];
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

int XWinFrameProcessor::convertYuyv(FrameBuffer buf, size_t len) {
    int c, d, e;

    if (len != (mImageWidth * mImageHeight * 2)) {
        std::cout << "Expected " << (mImageWidth * mImageHeight * 2) << " bytes"
            << " and received " << len << " bytes" << std::endl;
        return -1;
    }

    std::memcpy(mTempImageBuffer, buf, len);

    if (mMonochrome) {
        for (size_t src = 0; src < len; src++) {
            if ((src % 2) == 1) {
                mTempImageBuffer[src] = 128;
            }
        }
    }

    for (size_t src = 0, dst = 0; src < len; src += 4, dst += 8) {
        d = (int) mTempImageBuffer[src + 1] - 128;    // d = u - 128;
        e = (int) mTempImageBuffer[src + 3] - 128;    // e = v - 128;
        // c = y’ - 16 (for first pixel)
        c = 298 * ((int) mTempImageBuffer[src] - 16);
        // B - Blue
        mImageBuffer[dst] = clamp((c + 516 * d + 128) >> 8);
        // G -Green
        mImageBuffer[dst + 1] = clamp((c - 100 * d - 208 * e + 128) >> 8);
        // R - Red
        mImageBuffer[dst + 2] = clamp((c + 409 * e + 128) >> 8);

        // c = y’ - 16 (for second pixel)
        c = 298 * ((int ) mTempImageBuffer[src + 2] - 16);
        // B - Blue
        mImageBuffer[dst + 4] = clamp((c + 516 * d + 128) >> 8);
        // G -Green
        mImageBuffer[dst + 5] = clamp((c - 100 * d - 208 * e + 128) >> 8);
        // R - Red
        mImageBuffer[dst + 6] = clamp((c + 409 * e + 128) >> 8);
    }
    return 0;
}

void XWinFrameProcessor::slidingWindow() {
    int x, y, k;

    float range = mOrientationOutput->mAngularPositionMax - mOrientationOutput->mAngularPositionMin;
    float position = mOrientationOutput->getAngularPosition();
    float percent = (position - mOrientationOutput->mAngularPositionMin) / range;
    int center = mImageWidth * percent;
    // min can go negative
    int min = (center - mImageWidth / 4);
    int max = (center + mImageWidth / 4);

    for(k = y = 0; y < (int) mImageHeight; y++) {
	    for(x = 0; x < (int) mImageWidth; x++, k += 4) {
            if ((x < min) || (x > max)) {
                mImageBuffer[k+0]=0;
                mImageBuffer[k+1]=0;
                mImageBuffer[k+2]=0;
            }
        }
    }
}

int XWinFrameProcessor::computeTrackingRGB() {
    unsigned int x, y, k;
    unsigned int count = 0;
    unsigned int sum = 0;
    int rv = -1;

    for(k = y = 0; y < mImageHeight; y++) {
	    for(x = 0; x < mImageWidth; x++, k += 4) {
            int b = (mImageBuffer[k+0] - mImageTrackingRGB[2]) * (mImageBuffer[k+0] - mImageTrackingRGB[2]);
            int g = (mImageBuffer[k+1] - mImageTrackingRGB[1]) * (mImageBuffer[k+1] - mImageTrackingRGB[1]);
            int r = (mImageBuffer[k+2] - mImageTrackingRGB[0]) * (mImageBuffer[k+2] - mImageTrackingRGB[0]);
            int delta = b + g + r;
            if (delta < 512) {
                count++;
                sum += x;
            }
        }
    }
    if (count > 128) {
        rv = sum / count;
    }
    return rv;
}

void XWinFrameProcessor::renderImage() {
    int err;

    XPutImage(mDisplay, mWindow, mContext, mImage, 0, 0, 0, 0, mImageWidth, mImageHeight);
    err = errno;
    XFlush(mDisplay);
    errno = err;
}

XWinFrameProcessor::XWinFrameProcessor(const Options& options,
    OrientationInput* orientationInput, OrientationOutput const* orientationOutput) :

    FrameProcessor(options.mVideoType),
    mOrientationInput(orientationInput),
    mOrientationOutput(orientationOutput),
    mImageWidth(options.mImageWidth), mImageHeight(options.mImageHeight),
    mMonochrome(options.mImageMonochrome),
    mImageSlidingWindow(options.mImageSlidingWindow),
    mImageTrackingFrameCount(0)
{
    mImageTrackingRGB[0] = options.mImageTrackingRGB[0];
    mImageTrackingRGB[1] = options.mImageTrackingRGB[1];
    mImageTrackingRGB[2] = options.mImageTrackingRGB[2];    
}

XWinFrameProcessor::~XWinFrameProcessor()
{
    term();
}

int XWinFrameProcessor::init()
{
    return xwinDisplayInitialize();
}

void XWinFrameProcessor::term()
{
    xwinDisplayTerminate();
}

void XWinFrameProcessor::trackRGB() {
    int center = mImageWidth / 2;
    int delta;

    mImageTrackingFrameCount++;
    int inputAngle = mOrientationInput->getAngularPosition();
    int outputAngle = mOrientationOutput->getAngularPosition();
    if (inputAngle != outputAngle) {
        // motor is currently moving
        return;
    }
    int objectPosition = computeTrackingRGB();
    if (objectPosition < 0) {
        // object not found
        return;
    }
    if (mImageSlidingWindow) {
        float range = mOrientationOutput->mAngularPositionMax - mOrientationOutput->mAngularPositionMin;
        float percent = (outputAngle - mOrientationOutput->mAngularPositionMin) / range;
        center = mImageWidth * percent;        
    }
    delta = objectPosition - center;
    if (delta > 64) {
        mOrientationInput->setAngularPosition(inputAngle + 1);
    } else if (delta < -64) {
        mOrientationInput->setAngularPosition(inputAngle - 1);
    }
}

int XWinFrameProcessor::processFrame(FrameBuffer data, size_t length)
{
    int rv;
    switch (mVideoType) {
        case JPEG:
            rv = convertJpeg(data, length);
            break;
        case YUYV:
            rv = convertYuyv(data, length);
            break;
        default:
            std::cout << "Unknown video type " << mVideoType << std::endl;
            return -1;
    }
    if (rv) {
        return rv;
    }
    trackRGB();
    if (mImageSlidingWindow) {
        slidingWindow();
    }
    renderImage();
    return 0;
}
