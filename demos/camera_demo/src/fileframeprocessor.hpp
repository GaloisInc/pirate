#pragma once

#include <string>
#include "frameprocessor.hpp"

class FileFrameProcessor : public FrameProcessor
{
public:
    FileFrameProcessor(const Options& options);
    virtual ~FileFrameProcessor();

    virtual int init() override;
    virtual void term() override;

protected:
    virtual int processFrame(FrameBuffer data, size_t length) override;
    virtual unsigned char* getFrame(unsigned index, VideoType videoType) override;

private:
    const std::string mOutputDirectory;
    const bool mVerbose;
};

