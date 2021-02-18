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
 * Copyright 2021 Two Six Labs, LLC.  All rights reserved.
 */

#pragma once

#include <string>

#include "frameprocessor.hpp"
#include "imageconvert.hpp"

class MetaDataOpenLayers : public FrameProcessor
{
public:
    MetaDataOpenLayers(const Options& options);
    virtual ~MetaDataOpenLayers();

    virtual int init() override;
    virtual void term() override;

protected:
    virtual int process(FrameBuffer data, size_t length, DataStreamType dataStream) override;

private:
    const std::string  mOpenLayersApiUrl;

    float mLatitude, mLongitude;

    void sendLocation();
};

