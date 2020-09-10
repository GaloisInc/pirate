#pragma once

#include <endian.h>
#include <cstring>
#include <vector>
#include <cstdint>
#include <optional>

template<typename T> struct Serialize {};

/*
template<>
struct Serialize<A> {
    static constexpr size_t size = N;
    static A fromBuffer(std::vector<char> const& buffer);
    static void toBuffer(std::vector<char> & buffer, A const& str);
};
*/

template<typename T>
inline T deserialize(char const* buffer) {
    return Serialize<T>::fromBuffer(buffer);
}

template<typename T>
inline void serialize(std::vector<char> &buffer, T const& x) {
    Serialize<T>::toBuffer(buffer, x);
}

template<>
struct Serialize<uint64_t> {
    static constexpr size_t size = sizeof(uint64_t);
    static uint64_t fromBuffer(char const* buffer) {
        uint64_t raw;
        memcpy(&raw, buffer, sizeof raw);
        return be64toh(raw);
    }
    static void toBuffer(std::vector<char> & buffer, uint64_t x) {
        x = htobe64(x);
        char bytes[sizeof x];
        memcpy(bytes, &x, sizeof x);
        std::copy(std::begin(bytes), std::end(bytes), std::back_inserter(buffer));
    }
};

template<typename T>
struct Serialize<std::optional<T>> {
    static constexpr size_t size = 1 + sizeof(T);
    static std::optional<T> fromBuffer(char const* buffer) {
        if (*buffer) {
            return std::optional{deserialize<T>(buffer+1)};
        } else {
            return std::optional<T>();
        }
    }
    static void toBuffer(std::vector<char> & buffer, std::optional<T> const& x) {
        if (x) {
            buffer.push_back(1);
            serialize(buffer, *x);
        } else {
            buffer.push_back(0);
            std::fill_n(std::back_inserter(buffer), Serialize<T>::size, 0);
        }
    }
};
