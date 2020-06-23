#pragma once
#include "libpirate.h"
#include "channel.h"
#include "print.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

/** Write the bytes to a file descriptor, and check that all bytes were written. */
void gdCheckedWrite(int gd, const void* buf, size_t n);

template<typename T>
Sender<T> gdSender(int gd) {
  auto sendFn = [gd](const T& d) { gdCheckedWrite(gd, &d, sizeof(T)); };
  auto closeFn = [gd]() {
    pirate_close(gd);
  };
  return Sender<T>(sendFn, closeFn);
}

template<typename T>
Sender<T> pirateSender(const std::string& config) {
    int gd = pirate_open_parse(config.c_str(), O_WRONLY);
    if (gd < 0) {
        channel_errlog([config](FILE* f) { fprintf(f, "Open %s failed (error = %d)", config.c_str(), errno); });
        exit(-1);
    }
    // Return sender
    return gdSender<T>(gd);
}

/**
 * Read messages from file descriptor.
 *
 * Note. This read is tailored to a blocking datgram interface
 * where we expect each call will read a precise number of bytes.
 */
template<typename T>
void gdDatagramReadMessages(int gd, std::function<void(const T&)> f)
{
  while (true) {
    T x;
    ssize_t cnt = pirate_read(gd, &x, sizeof(T));
    if (cnt == -1) {
      char config[128];
      pirate_get_channel_description(gd, config, sizeof(config));
      channel_errlog([config](FILE* f) { fprintf(f, "Read %s failed (error = %d)", config, errno); });
      exit(-1);
    }
    if (cnt == 0) {
      break;
    }
    if (cnt != sizeof(T)) {
      char config[128];
      pirate_get_channel_description(gd, config, sizeof(config));
      channel_errlog([config, cnt](FILE* f) { fprintf(f, "Read %s incorrect bytes (expected = %lu, received = %lu)", config, sizeof(T), cnt); });
      exit(-1);
    }
    f(x);
  }
  pirate_close(gd);
}

template<typename T>
Receiver<T> gdReceiver(int gd) {
  auto receiveFn = [gd](std::function<void (const T& d)> fn) {
    gdDatagramReadMessages<T>(gd, fn);
  };
  return Receiver<T>(receiveFn);
}

template<typename T>
Receiver<T> pirateReceiver(const std::string& config) {
  int gd = pirate_open_parse(config.c_str(), O_RDONLY);
  if (gd < 0) {
    channel_errlog([config](FILE* f) { fprintf(f, "Open %s failed (error = %d)", config.c_str(), errno); });
    exit(-1);
  }
  // Return sender
  return gdReceiver<T>(gd);
}
