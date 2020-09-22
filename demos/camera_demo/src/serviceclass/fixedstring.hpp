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
    static FixedString<N> fromBuffer(char const* buffer) {
        auto end = std::find(buffer, buffer+N, 0);
        return std::string(buffer, end);
    }
    static void toBuffer(std::vector<char> & buffer, FixedString<N> const& str) {
        std::copy(std::begin(str.str), std::end(str.str), std::back_inserter(buffer));
        ssize_t zeros = N - static_cast<ssize_t>(str.str.size());
        std::fill_n(std::back_inserter(buffer), zeros, 0);
    }
};