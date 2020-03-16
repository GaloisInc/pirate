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
  std::thread t(r, f);
  t.detach();
}

template<typename T>
struct SenderReceiverPair {
  Sender<T> sender;
  Receiver<T> receiver;
};