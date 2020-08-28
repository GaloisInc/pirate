#pragma once

#include <string>
#include "frameprocessor.hpp"

class FileFrameProcessor : public FrameProcessor
{
public:
    FileFrameProcessor(VideoType videoType, const std::string& outputPath, bool verbose = false);
    virtual ~FileFrameProcessor();

    virtual int init();
    virtual void term();
    virtual int processFrame(FrameBuffer data, size_t length);

private:
    const std::string mOutputDirectory;
    const bool mVerbose;
};

