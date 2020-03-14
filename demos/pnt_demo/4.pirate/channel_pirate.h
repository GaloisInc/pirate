#pragma once
#include "channel.h"
#include <libpirate.h>

template<typename T>
void pirateReadMessages(const char* nm, int gd, std::function<void (const T& d)> f)
{
  char bytes[sizeof(T)];
  size_t off = 0;
  while (true) {
    ssize_t cnt = pirate_read(gd, bytes + off, sizeof(T) - off);
    if (cnt == -1) {
      print([nm](std::ostream& o) { o << "Read " << nm << " failed " << errno << std::endl; });
      exit(-1);
    }
    if (cnt == sizeof(T) - off) {
      f(*(reinterpret_cast<T*>(bytes)));
      off = 0;
    } else {
      print([cnt](std::ostream& o) { o << "Read " << cnt << " bytes." << std::endl; });
      off += cnt;
    }
  }
}

template<typename T>
Receiver<T> pirateReceiver(int rd, const std::string& rdparam, const char* nm)
{
  if (rdparam == "") {
    print([nm](std::ostream& o) { o << nm << " missing." << std::endl; });
    exit(-1);
  }
  pirate_channel_param_t param;
  if (pirate_parse_channel_param(rdparam.c_str(), &param) != 0) {
    print([nm](std::ostream& o) { o << nm << " parse parameter failed." << std::endl; });
    exit(-1);
  }

  if (pirate_set_channel_param(rd, O_RDONLY, &param) != 0) {
    print([nm](std::ostream& o) { o << nm << " set parameter failed." << std::endl; });
    std::cerr << "channel parameter set failed" << std::endl;
    exit(-1);
  }

  print([nm](std::ostream& o) { o << nm << " receiver try open." << std::endl; });
  int rdGD = pirate_open(rd, O_RDONLY);
  if (rdGD == -1) {
    print([nm](std::ostream& o) { o << nm << " receiver open failed " << errno << std::endl; });
    exit(-1);
  }
  print([nm](std::ostream& o) { o << nm << " receiver open." << std::endl; });

  return
    [nm, rdGD](std::function<void (const T& d)> f) {
      print([nm](std::ostream& o) { o << nm << " receiver called." << std::endl; });
      pirateReadMessages<T>(nm, rdGD, f);
    };
}

template<typename T>
Sender<T> pirateSender(int wr, const std::string& wrparam, const char* nm)
{
  if (wrparam == "") {
    std::cerr << "Specify " << nm << std::endl;
    exit(-1);
  }

  pirate_channel_param_t param;
  if (pirate_parse_channel_param(wrparam.c_str(), &param)) {
    perror("channel parameter set failed");
    exit(-1);
  }

  if (pirate_set_channel_param(wr, O_WRONLY, &param) < 0) {
    perror("channel parameter set failed");
    exit(-1);
  }

  print([nm](std::ostream& o) { o << nm << " sender try open." << std::endl; });
  int wrGD = pirate_open(wr, O_WRONLY);

  if (wrGD == -1) {
    print([nm](std::ostream& o) { o << nm << " sender open failed" << std::endl; });
    exit(-1);
  }
  print([nm](std::ostream& o) { o << nm << " sender open." << std::endl; });


  return [nm,wrGD](const T& d) {
      int cnt = pirate_write(wrGD, &d, sizeof(T));
      print([nm](std::ostream& o) { o << "Write " << nm << " " << sizeof(T) << " bytes." << std::endl; });
      if (cnt == -1) {
        print([nm](std::ostream& o) { o << "Write to " << nm << " failed " << errno << std::endl; });
        exit(-1);
      }
      if (cnt != sizeof(T)) {
        print([nm](std::ostream& o) { o << "Write to " << nm << " had unexpected count." << std::endl; });
        exit(-1);
      }
    };
}
