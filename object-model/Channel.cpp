#include "Channel.hpp"
#include "libpirate.h"
#include <unistd.h>

Channel::Channel(int *gd) : gd(gd) {}

void Channel::sendRaw(event_id ev, char const* bytes, std::size_t len) const {
    pirate_write(*gd, &ev, sizeof ev);
    pirate_write(*gd, &len, sizeof len);
    pirate_write(*gd, bytes, len);
}

event_id Channel::recvRaw(std::vector<char> *buffer) const {
    event_id ev;
    pirate_read(*gd, &ev, sizeof ev);

    std::size_t len;
    pirate_read(*gd, &len, sizeof len);
            
    buffer->resize(len);
    pirate_read(*gd, buffer->data(), len);

    return ev;
}

int Channel::recvWaitFd() const {
    return pirate_get_fd(*gd);
}

bool operator==(Channel const& lhs, Channel const& rhs) {
    return *lhs.gd == *rhs.gd;
}
