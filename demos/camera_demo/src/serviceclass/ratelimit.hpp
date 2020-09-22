#pragma once

#include <chrono>

class RateLimit {
    std::chrono::steady_clock::duration cost;
    std::chrono::steady_clock::duration grace;
    std::chrono::steady_clock::time_point cutoff;

public:
    using clock = std::chrono::steady_clock;

    explicit RateLimit(clock::duration cost, unsigned burst)
    : cost(cost)
    , grace(burst * cost)
    , cutoff()
    {}

    clock::duration delay_needed() const {
        auto now = clock::now();
        auto d = cutoff - now;
        return d > grace ? d - grace : clock::duration();
    }

    void mark() {
        auto now = clock::now();
        if (cutoff < now) { cutoff = now; }
        cutoff += cost;
    }
};