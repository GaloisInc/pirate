#include <cerrno>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>
#include "xwinframeprocessor.hpp"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <jpeglib.h>

int XWinFrameProcessor::xwinDisplayInitialize() {
    mDisplay = XOpenDisplay(NULL);
    if (mDisplay == NULL) {
        std::cout << "Failed to open X display" << std::endl;
        return -1;
    }
    int x_screen = DefaultScreen(mDisplay);
    mWindow = XCreateSimpleWindow(mDisplay,
        RootWindow(mDisplay, x_screen), 10, 10, mImageWidth, mImageHeight, 1,
        BlackPixel(mDisplay, x_screen), WhitePixel(mDisplay, x_screen));
    mContext = XCreateGC(mDisplay, mWindow, 0, &mContextVals);
    mImageBuffer = (char*) calloc(mImageWidth * mImageHeight * 4, 1);
    mJpegBuffer = (unsigned char*) calloc(mImageWidth * mImageHeight * 3, 1);
    mJpegBufferRow = (unsigned char*) calloc(mImageWidth * 3, 1);
    mImage = XCreateImage(mDisplay, CopyFromParent, 24, ZPixmap, 0, mImageBuffer,
        mImageWidth, mImageHeight, 32, 4 * mImageWidth);
    XMapWindow(mDisplay, mWindow);
    XSync(mDisplay, 0);
    return 0;
}


void XWinFrameProcessor::xwinDisplayTerminate() {
    if (mDisplay != NULL) {
        XDestroyImage(mImage); // frees mImageBuffer
        XFreeGC(mDisplay, mContext);
        XCloseDisplay(mDisplay);
        free(mJpegBuffer);
        free(mJpegBufferRow);
    }
    mImage = NULL;
    mDisplay = NULL;
    mImageBuffer = NULL;
    mJpegBuffer = NULL;
    mJpegBufferRow = NULL;
    mContext = NULL;
}

void XWinFrameProcessor::convertJpeg(FrameBuffer buf, size_t len) {
    int i, width, depth;
    unsigned x, y, z, k;

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    JSAMPROW row_pointer[1];

    unsigned long location = 0;

    cinfo.err = jpeg_std_error(&jerr);

    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, (unsigned char *) buf, len);
    jpeg_read_header(&cinfo, 1);
    cinfo.scale_num = 1;
    cinfo.scale_denom = 1;

    jpeg_start_decompress(&cinfo);
    width = cinfo.output_width;
    depth = cinfo.num_components; //should always be 3

    row_pointer[0] = mJpegBufferRow;

    while(cinfo.output_scanline < cinfo.output_height) {
	    jpeg_read_scanlines(&cinfo, row_pointer, 1);
	    for(i = 0; i < (width * depth); i++) {
	        mJpegBuffer[location++] = row_pointer[0][i];
        }
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    for(z = k = y = 0; y < mImageHeight; y++) {
	    for(x = 0; x < mImageWidth; x++) {
		    // for 24 bit depth, organization BGRX
            mImageBuffer[k+0]=mJpegBuffer[z+2];
			mImageBuffer[k+1]=mJpegBuffer[z+1];
			mImageBuffer[k+2]=mJpegBuffer[z+0];
			k+=4; z+=3;
		}
	}
}

void XWinFrameProcessor::renderImage() {
    int err;

    XPutImage(mDisplay, mWindow, mContext, mImage, 0, 0, 0, 0, mImageWidth, mImageHeight);
    err = errno;
    XFlush(mDisplay);
    errno = err;
}

XWinFrameProcessor::XWinFrameProcessor(unsigned width, unsigned height) :
    mImageWidth(width), mImageHeight(height)
{

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

int XWinFrameProcessor::processFrame(FrameBuffer data, size_t length)
{
    convertJpeg(data, length);
    renderImage();
    return 0;
}
