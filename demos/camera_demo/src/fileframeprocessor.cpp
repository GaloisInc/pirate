#include <cerrno>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>
#include "fileframeprocessor.hpp"

FileFrameProcessor::FileFrameProcessor(std::string& outputPath, bool verbose) :
    mOutputDirectory(outputPath),
    mVerbose(verbose) 
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

int FileFrameProcessor::processFrame(FrameBuffer data, size_t length)
{
    // Save the image file
    std::stringstream ss;
    ss << mOutputDirectory << "/capture_" 
       << std::setfill('0') << std::setw(4) << mIndex << ".jpg";
    
    std::ofstream out(ss.str(), std::ios::out | std::ios::binary);
    if (!out)
    {
        std::perror("Failed to open image file for writing");
        return -1;   
    }

    out.write(data, length);
    out.close();

    if (!out.good())
    {
        std::perror("Failed to write image content");
        return -1;
    }

    if (mVerbose)
    {
        std::cout << ss.str() << std::endl;
    }

    return 0;
}

