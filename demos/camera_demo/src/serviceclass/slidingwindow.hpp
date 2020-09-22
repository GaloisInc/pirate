#pragma once

#include <vector>
#include <optional>
#include <algorithm>

template <typename T>
class SlidingWindow {
    std::vector<std::optional<T>> window;
    uint64_t low_id;
    size_t start;

public:
    explicit SlidingWindow(size_t n, uint64_t i = 0)
        : window(n)
        , low_id(i)
        , start(0)
    {}

    uint64_t next_id() const { return low_id; }
    bool ready() const { return window[start].has_value(); }
    std::optional<T> pop() {
        if (window[start]) {
            auto result = std::move(window[start]);
            window[start] = std::optional<T>();
            low_id++;
            start = (start + 1) % window.size();
            return result;
        } else {
            return std::optional<T>();
        }
    }

    void insert(uint64_t id, T t) {
        // ID too small
        if (id < low_id) { return; }

        // ID too big
        if (low_id + window.size() <= id) { 
            return;
        }

        window[ (start + (id - low_id)) % window.size()] = t;
    }
};