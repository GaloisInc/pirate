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
    virtual int process(FrameBuffer data, size_t length) override;
    virtual unsigned char* getFrame(unsigned index, VideoType videoType) override;

private:
    std::string buildFilename(unsigned index);

    const std::string mOutputDirectory;
    const unsigned mImageOutputMaxFiles;
    const bool mVerbose;
};

