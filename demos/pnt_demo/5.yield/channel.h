#pragma once
#include <functional>
#include <vector>
#include "libpirate.h"
#include "pnt_data.h"

class Listener {
protected:
  int _readCtrlGd, _readCtrlFd;
  int _writeCtrlGd;
public:
  Listener(int readCtrlGd, int writeCtrlGd) : 
    _readCtrlGd(readCtrlGd), _writeCtrlGd(writeCtrlGd) {
    _readCtrlFd = pirate_get_fd(readCtrlGd);
  }
  virtual void listen() = 0;
};

template<typename T>
class SendChannel {
private:
  Listener &_listener;
  std::vector<int> _gds;
public:
  SendChannel(Listener &listener) : _listener(listener) {}
  SendChannel(const SendChannel<T> &c) : _listener(c._listener) {}
  void add(int gd) { _gds.push_back(gd); }
  SendChannel& operator=(SendChannel& ) = delete;
  void send(const T& data) {
    for (int gd : _gds) {
      pirate_write(gd, &data, sizeof(data));
      _listener.listen(); // wait for control to come back
    }
  }
};

class GreenListener : public Listener {
private:
  int _uavGd, _uavFd;
  int _rfGd, _rfFd;
  int _gpsGd, _gpsFd;
public:
  GreenListener(int uavGd, int rfGd, int gpsGd, 
    int readCtrlGd, int writeCtrlGd) :
    Listener(readCtrlGd, writeCtrlGd),
    _uavGd(uavGd), _rfGd(rfGd), _gpsGd(gpsGd) {
      _uavFd = pirate_get_fd(uavGd);
      _rfFd = pirate_get_fd(rfGd);
      _gpsFd = pirate_get_fd(gpsGd);
    }
  // FIXME: callbacks are public to avoid circular dependency in constructor    
  std::function<void(const Position&)> _uavFunc;
  std::function<void(const Distance&)> _rfFunc;
  std::function<void(const Position&)> _gpsFunc;
  void listen();
};

class OrangeListener : public Listener {
private:
  int _gpsGd, _gpsFd;
public:
  // FIXME: callbacks are public to avoid circular dependency in constructor
  std::function<void(const Position&)> _gpsFunc;
  OrangeListener(int gpsGd, int readCtrlGd, int writeCtrlGd) :
    Listener(readCtrlGd, writeCtrlGd),
    _gpsGd(gpsGd) {
    _gpsFd = pirate_get_fd(gpsGd);
  }
  void listen();
};
