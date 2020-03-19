#pragma once
#include <functional>
#include <iostream>
#include <sstream>
#include <thread>
#include <unistd.h>

// This prints out the string written to ostream in a single
// system call.
inline
void print(std::function<void(std::ostream& o)> p) {
  std::stringstream o;
  p(o);
  std::string s=o.str();
  write(STDOUT_FILENO, s.c_str(), s.size());  
}

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
using Receiver = std::function<void(std::function<void (const T& d)>)>;

template<typename T>
std::thread asyncReadMessages(Receiver<T> r, std::function<void (const T& d)> f)
{
  return std::thread(r, f);
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
