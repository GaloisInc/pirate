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

#include <math.h>

#include "metadataopenlayers.hpp"

#include "orion-sdk/KlvParser.hpp"
#include "orion-sdk/KlvTree.hpp"

#include <cpprest/http_client.h>
using namespace web::http;
using namespace web::http::client;

MetaDataOpenLayers::MetaDataOpenLayers(const Options& options) :
    FrameProcessor(options.mVideoOutputType, options.mImageWidth, options.mImageHeight),
    mOpenLayersApiUrl(options.mOpenLayersApiUrl)
{

}

MetaDataOpenLayers::~MetaDataOpenLayers()
{
    term();
}

int MetaDataOpenLayers::init()
{
    return 0;
}

void MetaDataOpenLayers::term()
{
}

void MetaDataOpenLayers::sendLocation()
{
#ifdef RESTSDK_PRESENT
    web::json::value postData;
    postData["latitude"] = web::json::value::number(mLatitude);
    postData["longitude"] = web::json::value::number(mLongitude);
    http_client client = http_client(U(mOpenLayersApiUrl));
    client.request(methods::POST, U("/location"), postData).wait();
#endif
}

int MetaDataOpenLayers::process(FrameBuffer data, size_t length, DataStreamType dataStream)
{
    if (dataStream == MetaData) {
        int success1, success2;
        double lat, lon;
        KlvNewData(data, length);
        lat = KlvGetValueDouble(KLV_UAS_SENSOR_LAT, &success1);
        lon = KlvGetValueDouble(KLV_UAS_SENSOR_LON, &success2);
        if (success1 && success2) {
            mLatitude = lat * 180.0 / M_PI;
            mLongitude = lon * 180.0 / M_PI;
            sendLocation();
        }
    }

    return 0;
}
