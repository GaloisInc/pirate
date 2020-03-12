#pragma once
#include <vector>

template<typename T>
class SendChannel {
public:
  SendChannel(const SendChannel& c) = delete;
  SendChannel& operator=(SendChannel& ) = delete;
  virtual void send(const T& data) = 0;
};

template<typename T>
class CallbackChannel : public SendChannel<T> {
  std::vector<std::function<void(const T&)>> _listeners;

public:
  CallbackChannel() {}

  void send(const T& data) {
    for (auto l : _listeners) l(data);
  }

  void addListener(std::function<void(const T&)> l) {
    _listeners.push_back(l);
  }
};
