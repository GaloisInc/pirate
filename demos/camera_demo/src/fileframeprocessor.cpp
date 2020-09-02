#include <cerrno>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>

#include <stdio.h>

#include "fileframeprocessor.hpp"

FileFrameProcessor::FileFrameProcessor(const Options& options) :
    FrameProcessor(options.mVideoType, options.mImageWidth, options.mImageHeight),
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

unsigned char* FileFrameProcessor::getFrame(unsigned index, VideoType videoType) {
    (void) index;
    (void) videoType;
    return nullptr;
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

