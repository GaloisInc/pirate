#include "message.hpp"
#include "Channel.hpp"
#include "libpirate.h"
#include "pal/pal.h"

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
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#pragma pirate enclave declare(main)
#pragma pirate enclave declare(send)
#pragma pirate enclave declare(recv)

#ifndef __GAPS__
#error Needs gaps
#endif

using pirate::Serialization;

// ===========================================================
// CHANNEL LIBRARY - libpirate / launcher interface
// ===========================================================

struct Port {
    Channel c;
    event_id ev;

    Port(Channel c, event_id ev) : c(c), ev(ev) {}
};

template<typename T>
class OutPort : Port {
public:
    OutPort(Channel c, event_id ev) : Port(c,ev) {}
    void send(T const& msg) {
        std::vector<char> buf;
        Serialization<T>::toBuffer(msg, buf);
        c.sendRaw(ev, buf.data(), buf.size()); 
    }
};

template<typename T>
class InPort : public Port {
public:
    InPort(Channel c, event_id ev) : Port(c,ev) {}
};

struct NoChannel {};

class ChannelRegistry {
    std::unordered_map<std::string, std::pair<Channel, event_id>> channels;

public:
    template<typename T>
    std::pair<Channel, event_id> getEntry(std::string const& name) {
        auto it = channels.find(name);
        if (it == std::end(channels)) {
            throw NoChannel();
        }
        return {it->second.first, it->second.second};
    }

    void setEntry(std::string const& name, Channel c, event_id ev) {
        channels.insert(std::make_pair(name, std::make_pair(c, ev)));
    }
};

ChannelRegistry registry;

// ===========================================================
// LIBRARY CODE
// ===========================================================

class Handler {
    
public:    
    virtual ~Handler(){}
    virtual void run(std::vector<char> const&) = 0;
};

class CHandler : public Handler {
public:
    using callback_t = void(*)(void*, char const*, std::size_t);

    void * dat;
    callback_t callback;

public:
    CHandler() : dat(), callback() {}
    CHandler(void * dat, callback_t callback) : dat(dat), callback(callback) {}
    void run(std::vector<char> const& buffer) override {
        callback(dat, buffer.data(), buffer.size());
    }
};

template<typename T, typename M>
class MethodHandler : public Handler {
    T *t;
    void (T::*f)(M m);

public:
    MethodHandler(T *t, void (T::*f)(M m)):t(t), f(f){}
    void run(std::vector<char> const& buffer) override {
        M x = Serialization<M>::fromBuffer(buffer);
        (t->*f)(x);
    }
};

template<typename T, typename M>
MethodHandler<T, M> *make_methodhandler(T *t, void (T::*f)(M m)) {
    return new MethodHandler<T, M>(t,f);
}


class Dispatch {
    std::unordered_map<Channel, std::unordered_map<event_id, std::unique_ptr<Handler>>> handlers;
public:
    void registerReceiver(Port p, std::unique_ptr<Handler> h) {
    }
    
    // blocking mode
    bool step(){ 
        std::vector<Channel> chans;
        std::vector<struct pollfd> fds;

        if (handlers.empty()) {
            return false;
        }

        for (auto const& kv : handlers) {
            struct pollfd entry;
            entry.fd = kv.first.recvWaitFd();
            entry.events = POLLIN;
            entry.revents = 0;
            chans.push_back(kv.first);
            fds.push_back(entry);
        }

        poll(fds.data(), fds.size(), 0);

        for (size_t i = 0; i < fds.size(); i++) {
            struct pollfd &entry = fds[i];

            if (POLLIN & entry.revents) {
                std::vector<char> buffer;
                event_id ev = chans[i].recvRaw(&buffer);
                handlers[chans[i]][ev]->run(buffer);
            }
        }

        return true;
    }

    template<typename R, typename M>
    void registerReceiver(InPort<M> port, R *receiver, void (R::*callback)(M)) {
        handlers[port.c][port.ev] = std::unique_ptr<Handler>(make_methodhandler(receiver, callback));
    }

    template<typename M>
    void registerReceiver(InPort<M> port, CHandler::callback_t callback, void *dat) {
        handlers[port.c][port.ev] = std::unique_ptr<Handler>(new CHandler(dat, callback));
    }
};


// Adapter to serialize messages before they are sent over channels using the channel's native framing
template<typename T> void genericSend(event_id ev, T const& t, Channel *c) {
    std::vector<char> buf;
    Serialization<T>::toBuffer(t, buf);
    c->sendRaw(ev, buf.data(), buf.size());
}

// ===========================================================
// APPLICATION CODE
// ===========================================================


constexpr event_id EvPOSITION = 10;

struct BadPosition {};

// ===========================================================
// Sender enclave
// ===========================================================

pirate_channel outChannel
__attribute__((pirate_resource("out","send")))
__attribute__((pirate_resource_param("permissions", "writeonly")));

OutPort<om::position> positionOutPort(Channel(&outChannel), EvPOSITION);

int sender_main()
__attribute__((pirate_enclave_main("send")))
{
    
    std::cout << "Sender started\n";
    positionOutPort.send({2, 4, 8});
    std::cout << "Sender done\n";

    return 0;
}

// ===========================================================
// Receiver enclave
// ===========================================================

pirate_channel inChannel
__attribute__((pirate_resource("in","recv")))
__attribute__((pirate_resource_param("permissions", "readonly")));

InPort<om::position> positionInPort(Channel(&inChannel), EvPOSITION);

// Application-specific instance
class PositionReceiver {
public:
    void receive(om::position p) {
        std::cout << "Event received: " << p.x << " " << p.y << " " << p.z << std::endl;
    }
};


int receiver_main()
__attribute__((pirate_enclave_main("recv")))
{
    PositionReceiver eventReceiver;
    Dispatch dispatcher;

    // This probably can't be automated due to use of templates, we'd just make it easy to define.
    dispatcher.registerReceiver(positionInPort, &eventReceiver, &PositionReceiver::receive);

    // Application developer integrates event dispatch into app
    while(dispatcher.step()) {}

    return 0;
}

// ===========================================================
// Launcher
// ===========================================================


#define PIPE_PATH "tmp.fifo"

void clean() { unlink(PIPE_PATH); }

int main()
__attribute__((pirate_enclave_main("main")))
{
    atexit(clean);

    if (mkfifo(PIPE_PATH, 0666)) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }

    pid_t sender_pid = fork();
    if (0 == sender_pid) {
	outChannel = pirate_open_parse("pipe," PIPE_PATH, O_WRONLY);
        int result = sender_main();
        pirate_close(outChannel);
	exit(result);
    }

    pid_t receiver_pid = fork();
    if (0 == receiver_pid) {
	inChannel = pirate_open_parse("pipe," PIPE_PATH, O_RDONLY);
        int result = receiver_main();
        pirate_close(inChannel);
	exit(result);
    }
    
    wait(nullptr);
    wait(nullptr);

    unlink(PIPE_PATH);

    return 0;
}
