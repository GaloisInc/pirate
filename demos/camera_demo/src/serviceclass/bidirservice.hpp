#pragma once

#include "serialize.hpp"
#include "request.hpp"
#include "response.hpp"

#include <libpirate.h>

#include <vector>

////////////////////////////////////////////////////////////////////////
// Bi-directional Service library
////////////////////////////////////////////////////////////////////////

class RemoteException : public std::exception {
    virtual char const* what() const noexcept override {
        return "remote unhandled exception";
    }
};

template<typename Derived, typename Request, typename Response>
class BidirService {
    int readChan;
    int writeChan;
    uint64_t tx_next;

    inline Response interface(Request t) {
        return static_cast<Derived*>(this)->impl(t);
    }

public:
    BidirService() : tx_next(0) {}
    void setHandles(int read, int write);
    Response operator()(Request t);
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

        std::optional<Res> answer;
        try {
            answer = std::optional(interface(req.content));
        } catch (...) {
        }

        auto resp = Response<Res>(req.tx_id, answer);
        buffer.clear();
        serialize(buffer, resp);
        pirate_write(writeChan, buffer.data(), buffer.size());
    }
}
template<typename Derived, typename Req, typename Res>
Res BidirService<Derived, Req, Res>::operator()(Req t) {
    std::vector<char> buffer;
    uint64_t tx_id = tx_next++;
    serialize(buffer, Request<Req>(tx_id, t));
    pirate_write(writeChan, buffer.data(), buffer.size());

    buffer.clear();
    buffer.resize(Serialize<Response<Res>>::size);
    pirate_read(readChan, buffer.data(), buffer.size());
    std::optional<Res> resp = deserialize<Response<Res>>(buffer.data()).content;
    if (resp) {
        return *resp;
    } else {
        throw RemoteException();
    }
}
