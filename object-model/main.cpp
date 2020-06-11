#include <cstdlib>
#include <string>
#include <cstring>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <unistd.h>
#include <poll.h>

using event_id = std::uint32_t;

// ===========================================================
// LIBRARY CODE
// ===========================================================

template <typename T>
struct Serialization {
    static void toBuffer(T const&, std::vector<char> *) = 0;
    static T fromBuffer(std::vector<char> const* buf) = 0;
};

class Handler {
    
public:    
    virtual ~Handler(){}
    virtual void run(char const* buffer, std::size_t len) = 0;
};

class CHandler : public Handler {
public:
    using callback_t = void(*)(void*, char const*, std::size_t);

    void * dat;
    callback_t callback;

public:
    CHandler() : dat(), callback() {}
    CHandler(void * dat, callback_t callback) : dat(dat), callback(callback) {}
    void run(char const* buffer, std::size_t len) override {
        callback(dat, buffer, len);
    }
};

template<typename T, typename M>
class MethodHandler : public Handler {
    T *t;
    void (T::*f)(M m);

public:
    MethodHandler(T *t, void (T::*f)(M m)):t(t), f(f){}
    void run(char const* buffer, std::size_t len) override {
        M x = Serialization<M>::fromBuffer(buffer, len);
        (t->*f)(x);
    }
};

template<typename T, typename M>
MethodHandler<T, M> *make_methodhandler(T *t, void (T::*f)(M m)) {
    return new MethodHandler<T, M>(t,f);
}

struct Channel {
    std::uint32_t ev;
    int fd;

    Channel() : ev(0), fd(-1) {}
    Channel(std::uint32_t ev, int fd) : ev(ev), fd(fd) {}
    void sendRaw(char const* bytes, size_t len) {
        write(fd, &ev, sizeof ev);
        write(fd, &len, sizeof len);
        write(fd, bytes, len);
    }
};

class Dispatch {
    std::unordered_map<int, std::unordered_map<event_id, std::unique_ptr<Handler>>> handlers;
public:
    void registerReceiver(Channel const& c, std::unique_ptr<Handler> h) {
        handlers[c.fd][c.ev] = std::move(h);
    }
    
    // blocking mode
    bool step(){ 
        std::vector<struct pollfd> fds;

        if (handlers.empty()) {
            return false;
        }

        for (auto const& kv : handlers) {
            struct pollfd entry;
            entry.fd = kv.first;
            entry.events = POLLIN;
            entry.revents = 0;
            fds.push_back(entry);
        }

        poll(fds.data(), fds.size(), 0);

        for (auto const& entry : fds) {

            if (POLLIN & entry.revents) {
                event_id ev;
                read(entry.fd, &ev, sizeof ev);

                std::size_t len;
                read(entry.fd, &len, sizeof len);
                
                std::vector<char> buffer(len);
                read(entry.fd, buffer.data(), len);

                handlers[entry.fd][ev]->run(buffer.data(), len);
            }

            if (POLLHUP & entry.revents) {
                close(entry.fd);
                handlers.erase(entry.fd);
            }
        }

        return true;
    }

    template<typename R, typename M>
    void registerMethod(Channel const& c, R *receiver, void (R::*callback)(M)) {
        registerReceiver(c, std::unique_ptr<Handler>(make_methodhandler(receiver, callback)));
    }
};

// Adapter to serialize messages before they are sent over channels using the channel's native framing
template<typename T> void genericSend(T const& t, Channel *c) {
    std::vector<char> buf;
    Serialization<T>::toBuffer(t, &buf);
    c->sendRaw(buf.data(), buf.size());
}

// ===========================================================
// APPLICATION CODE
// ===========================================================

Channel channel;

constexpr event_id EvPOSITION = 10;

struct Position {
    double x, y, z;
    Position() : x(), y(), z() {}
    Position(double x, double y, double z) : x(x), y(y), z(z) {}
};

struct BadPosition {};

// The developer will need to pick a serializer
template <>
struct Serialization<Position> {
    static void toBuffer(Position const& p, std::vector<char> *buf) {
        buf->resize(sizeof p);
        memcpy(buf->data(), &p, sizeof p);
    }
    static Position fromBuffer(char const* buf, size_t sz) {
        Position p;
        if (sz != sizeof p) throw(BadPosition());
        memcpy(&p, buf, sz);
        return p;
    }
};

// ===========================================================
// Sender enclave
// ===========================================================

class PositionSender {

public:
    void send(Position p) { genericSend(p, &channel); }
};

int sender_main() {
    
    PositionSender eventSender;

    eventSender.send(Position(2, 4, 8));

    return 0;
}

// ===========================================================
// Receiver enclave
// ===========================================================

// Application-specific instance
class PositionReceiver {
public:
    void receive(Position p) {
        std::cout << "Event received: " << p.x << " " << p.y << " " << p.z << std::endl;
    }
};


PositionReceiver eventReceiver; // Incoming channel name attribute: position1

Dispatch dispatcher;

int receiver_main() {
    // This probably can't be automated due to use of templates, we'd just make it easy to define.
    dispatcher.registerMethod(channel, &eventReceiver, &PositionReceiver::receive);

    // Application developer integrates event dispatch into app
    while(dispatcher.step()) {}

    return 0;
}

// ===========================================================
// Launcher
// ===========================================================

int main() {

    int fds[2];
    if (pipe(fds)) {
        std::cerr << "Failed to open pipe: " << strerror(errno) << std::endl;
        return 1;
    }

    pid_t sender_pid = fork();
    if (0 == sender_pid) {
        channel = Channel(EvPOSITION, fds[1]);
        close(fds[0]);

        return sender_main();
    }

    pid_t receiver_pid = fork();
    if (0 == receiver_pid) {
        channel = Channel(EvPOSITION, fds[0]);
        close(fds[1]);

        return receiver_main();
    }

    close(fds[0]);
    close(fds[1]);
    
    wait(nullptr);
    wait(nullptr);

    return 0;
}
