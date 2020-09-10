#pragma once

#include "serialize.hpp"
#include "request.hpp"
#include "response.hpp"

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
    void setHandles(int read, int write);
    Response operator()(Request t) const;
    int event_loop();
};

template<typename Derived, typename Request, typename Response>
void BidirService<Derived, Request, Response>::setHandles(int read, int write) {
    readChan = read;
    writeChan = write;
}

template<typename Derived, typename Req, typename Res>
int BidirService<Derived, Req, Res>::event_loop() {
    std::vector<char> buffer;
    for (;;) {
        buffer.clear();
        buffer.resize(Serialize<Request<Req>>::size);
        pirate_read(readChan, buffer.data(), buffer.size());
        auto req = deserialize<Request<Req>>(buffer.data());

        auto res = interface(req.content);
        auto resp = Response<Res>(req.tx_id, res);
        buffer.clear();
        serialize(buffer, resp);
        pirate_write(writeChan, buffer.data(), buffer.size());
    }
}
template<typename Derived, typename Req, typename Res>
Res BidirService<Derived, Req, Res>::operator()(Req t) const {
    std::vector<char> buffer;
    serialize(buffer, Request<Req>(0, t));
    pirate_write(writeChan, buffer.data(), buffer.size());

    buffer.clear();
    buffer.resize(Serialize<Response<Res>>::size);
    pirate_read(readChan, buffer.data(), buffer.size());
    Res resp = *(deserialize<Response<Res>>(buffer.data()).content);
    return resp;
}
