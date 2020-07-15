#pragma once

#include <cstdint>
#include <cstdlib>
#include <vector>
#include <functional>

using event_id = std::uint32_t;

class Channel {
    int* gd;

public:
    explicit Channel(int* gd);

    void sendRaw(event_id ev, char const* bytes, std::size_t len) const;
    event_id recvRaw(std::vector<char> *buffer) const;
    int recvWaitFd() const;

    friend struct std::hash<Channel>;
    friend bool operator==(Channel const& lhs, Channel const& rhs);
};

bool operator==(Channel const& lhs, Channel const& rhs);

namespace std {
    template<> struct std::hash<Channel> {
        std::size_t operator()(Channel const& c) const noexcept {
            return std::hash<int>{}(*c.gd);
        }
    };
}
