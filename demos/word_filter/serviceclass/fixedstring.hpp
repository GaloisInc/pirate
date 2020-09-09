#pragma once

#include "serialize.hpp"

#include <algorithm>
#include <string>
#include <vector>

template<int N>
struct FixedString {
    FixedString(std::string str) : str(str) {}
    operator std::string() const { return str; }
    std::string str;
};

template<int N>
struct Serialize<FixedString<N>> {
    static constexpr size_t size = N;
    static FixedString<N> fromBuffer(std::vector<char> const& buffer) {
        auto start = std::begin(buffer);
        auto len   = std::min(size, buffer.size());
        auto end   = std::find(start, start+len, 0);
        return std::string(std::begin(buffer), end);
    }
    static void toBuffer(std::vector<char> & buffer, FixedString<N> const& str) {
        buffer.clear();
        buffer.insert(buffer.end(), str.str.begin(), str.str.end());
        buffer.resize(size);
    }
};