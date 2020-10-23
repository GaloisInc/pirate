#pragma once

#include "serialize.hpp"
#include <endian.h>
#include <cstdint>

template <typename T>
struct Request {
    Request(uint64_t id, T t) : tx_id(id), content(t) {}
    uint64_t tx_id;
    T content;
};

template<typename T>
struct Serialize<Request<T>> {
    static constexpr size_t size = sizeof(uint64_t) + Serialize<T>::size;
    static Request<T> fromBuffer(char const* buffer) {
        return Request<T>(
            deserialize<uint64_t>(buffer),
            deserialize<T>(buffer + Serialize<uint64_t>::size));
    }
    static void toBuffer(std::vector<char> & buffer, Request<T> const& req) {
        serialize(buffer, req.tx_id);
        serialize(buffer, req.content);
    }
};