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

#include <thread>
#include <optional>
#include <algorithm>

#include "orientationinput.hpp"
#include "frameprocessor.hpp"
#include "options.hpp"
#include "colortracking.hpp"
#include "serviceclass/bidirservice.hpp"

// Fragmentation size likely needs to be statically known
// in order for it to be specified in IDL
constexpr size_t fragment_size = 1024;

// This message will be defined in IDL
struct Req {
    uint8_t buffer[fragment_size];
    size_t len;
    bool final_fragment;
};

// This message will be defined in IDL
struct Res {
    Res() : angular(), result() {}
    std::optional<float> angular;
    int result;
};


// Stub serialization; generated from IDL
template<>
struct Serialize<Req> {
    static constexpr size_t size = sizeof(Req);
    static Req fromBuffer(char const* buffer){ (void)buffer; for(;;){} }
    static void toBuffer(std::vector<char> & buffer, Req const& str){ (void)buffer; (void)str; }
};

// Stub serialization; generated from IDL
template<>
struct Serialize<Res> {
    static constexpr size_t size = sizeof(Res);
    static Res fromBuffer(char const* buffer){(void)buffer; for(;;){}}
    static void toBuffer(std::vector<char> & buffer, Res const& str){(void)buffer; (void)str;}
};



struct ColorTrackingEnclave : public BidirService<ColorTrackingEnclave, Req, Res> 
{
    ColorTracking *colortracking;
    std::optional<float> angular;
    std::vector<uint8_t> buffer;

    Res impl(Req req) {
        std::copy_n(std::begin(req.buffer), req.len, std::back_inserter(buffer));

        Res res;

        if (req.final_fragment) {
            angular.reset();
            res.result = colortracking->processFrame(buffer.data(), buffer.size());
            buffer.clear();
            res.angular = angular;
        }

        return res;
    }
} service
__attribute__((
// pirate_resource("colorenclave", "service")
));

// Shadow implementation of color tracking class
// that dispatchs to an implementation running on
// a remote enclave
//
// This needs to be a separate class from the service
// so that the class inhieretance can make sense for
// both the service class and the shadow class.
class ColorTrackingRemote : public OrientationInput, public FrameProcessor {

    // These don't do anything for the ColorTracking class, but if they did we'd have to marshal them
    int init() override { return 0; }
    void term() override {}
    unsigned char* getFrame(unsigned index, VideoType videoType) override {
        (void) index; (void) videoType; return nullptr;
    }

    int process(FrameBuffer data, size_t length) override {
        // Manual fragmentation management
        while (true) {
            Req req;
            req.len = length > fragment_size ? fragment_size : length;
            req.final_fragment = length == req.len;
            std::copy_n(data, req.len, req.buffer);
            data += req.len;
            length -= req.len;
            auto res = service(req);
            if (req.final_fragment) {
                if (res.angular.has_value()) {
                    setAngularPosition(res.angular.value());
                }
                return res.result;
            }
        }
    }

public:
    ColorTrackingRemote(AngularPosition<float>::UpdateCallback angPosUpdateCallback)
    : OrientationInput(angPosUpdateCallback)
    , FrameProcessor(VideoType::JPEG, 640, 480)
    {}
};

int enclave_main() {
    Options options;
    service.colortracking = new ColorTracking(options,
        // This class makes use of uni-directional callbacks; we have to emulate them
        [](float x){service.angular = x; return false;
        });

    service.event_loop();
    return 0;
} 
