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
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>

#include <stdio.h>

#include "fileframeprocessor.hpp"

FileFrameProcessor::FileFrameProcessor(const Options& options) :
    FrameProcessor(options.mVideoOutputType, options.mImageWidth, options.mImageHeight),
    mOutputDirectory(options.mImageOutputDirectory),
    mImageOutputMaxFiles(options.mImageOutputMaxFiles),
    mVerbose(options.mVerbose)
{

}

FileFrameProcessor::~FileFrameProcessor()
{
    term();
}

int FileFrameProcessor::init()
{
    return 0;
}

void FileFrameProcessor::term()
{

}

std::string FileFrameProcessor::buildFilename(unsigned index) {
    std::stringstream ss;

    ss << mOutputDirectory << "/capture_" 
       << std::setfill('0') << std::setw(4) << index;
    switch (mVideoType) {
        case JPEG:
            ss << ".jpg";
            break;
        case YUYV:
            ss << ".raw";
            break;
        case H264:
            ss << ".h264";
            break;
        default:
            std::cout << "Unknown video type " << mVideoType << std::endl;
            return "";
    }

    return ss.str();
}

int FileFrameProcessor::process(FrameBuffer data, size_t length)
{
    // Save the image file
    std::string filename = buildFilename(mIndex);
    if (filename.empty())
    {
        return -1;
    }
    
    std::ofstream out(filename, std::ios::out | std::ios::binary);
    if (!out)
    {
        std::perror("Failed to open image file for writing");
        return -1;   
    }

    out.write((const char*) data, length);
    out.close();

    if (!out.good())
    {
        std::perror("Failed to write image content");
        return -1;
    }

    if ((mImageOutputMaxFiles > 0) && (mIndex > mImageOutputMaxFiles)) {
        unsigned prevIndex = mIndex - mImageOutputMaxFiles;
        std::string prevFilename = buildFilename(prevIndex);
        if (!prevFilename.empty()) {
            remove(prevFilename.c_str());
        }
    }

    if (mVerbose)
    {
        std::cout << filename << std::endl;
    }

    return 0;
}

