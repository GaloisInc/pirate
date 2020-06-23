#pragma once
#include <functional>
#include <thread>

template<typename T>
class Sender {
  std::function<void(const T&)> _send;
  std::function<void(void)> _close;
public:
  Sender(const std::function<void(const T&)> send, const std::function<void(void)>& close)
    : _send(send), _close(close) {

    }

  void operator()(const T& x) const {
    _send(x);
  }
  void close(void) { _close(); }
};

template<typename T>
class Receiver {
  std::function<void(std::function<void (const T& d)>)> _receive;
public:
  Receiver(const std::function<void(std::function<void (const T& d)>)> receive)
    : _receive(receive) {
    }

  std::function<void(std::function<void (const T& d)>)> receiver() {
    return _receive;
  }

//  void close(void) { _close(); }
};

// std::function<void (const T& d)>

template<typename T, typename U>
std::thread startReadMessages(Receiver<T> r, U f)
{
  return std::thread(r.receiver(), f);
}

template<typename T>
struct SenderReceiverPair {
  Sender<T> sender;
  Receiver<T> receiver;
};

/** A function that given an ostream populates it with output. */
using Printer=std::function<void(std::ostream&)>;

/** A function that outputs the string returned by a printer atomically. */
using PrintSink=std::function<void(Printer)>;
