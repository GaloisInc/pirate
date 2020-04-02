#pragma once
#include <functional>
#include <vector>
#include "libpirate.h"
#include "pnt_data.h"

template<typename T>
class SendChannel {
private:
  std::vector<int> _gds;
public:
  SendChannel(std::vector<int> gds) : _gds(gds) {}
  SendChannel() = delete;
  SendChannel& operator=(SendChannel&) = delete;
  void send(const T& data) {
    for (int gd : _gds) {
      pirate_write(gd, &data, sizeof(data));
    }
  }
};
