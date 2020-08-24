#pragma once

#include "serialize.hpp"

#include <libpirate.h>

#include <vector>

////////////////////////////////////////////////////////////////////////
// Bi-directional Service library
////////////////////////////////////////////////////////////////////////

template<typename Derived, typename Request, typename Response>
class BidirService {
    int readChan;
    int writeChan;

    inline Response interface(Request t) {
        return static_cast<Derived*>(this)->impl(t);
    }

public:
    void setHandles(int read, int write) { readChan = read; writeChan = write; }

    Response operator()(Request t) const {
        std::vector<char> buffer;
        Serialize<Request>::toBuffer(buffer, t);
        pirate_write(writeChan, buffer.data(), buffer.size());

        buffer.clear();
        buffer.resize(80);
	pirate_read(readChan, buffer.data(), buffer.size());

        return Serialize<Response>::fromBuffer(buffer);
    }

    int event_loop() {
        std::vector<char> buffer;
        for (;;) {
            buffer.clear();
            buffer.resize(80);
            pirate_read(readChan, buffer.data(), buffer.size());
            auto req = Serialize<Request>::fromBuffer(buffer);

            auto res = interface(req);
            buffer.clear();
            Serialize<Response>::toBuffer(buffer, res);
            pirate_write(writeChan, buffer.data(), buffer.size());
        }
    }
};
