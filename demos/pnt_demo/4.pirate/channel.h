#pragma once
#include <functional>
#include <thread>
#include "print.h"

template<typename T>
using Sender = std::function<void(const T& m)>;

template<typename T>
using Receiver = std::function<void(std::function<void (const T& d)>)>;

template<typename T>
void asyncReadMessages(Receiver<T> r, std::function<void (const T& d)> f)
{
    print([](std::ostream& o) { o << "asyncReadMessages 0" << std::endl; });
    std::thread t(r, f);
    print([](std::ostream& o) { o << "asyncReadMessages 1" << std::endl; });
    t.detach();
    print([](std::ostream& o) { o << "asyncReadMessages 2" << std::endl; });
}

template<typename T>
struct SenderReceiverPair {
  Sender<T> sender;
  Receiver<T> receiver;
};