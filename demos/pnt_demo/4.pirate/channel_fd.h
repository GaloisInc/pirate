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
void gdCheckedWrite(const std::string& config, int gd, const void* buf, size_t n);
void piratePipe(const std::string& config, int gd);

template<typename T>
Sender<T> gdSender(const std::string& config, int gd) {
  auto sendFn = [config, gd](const T& d) { gdCheckedWrite(config, gd, &d, sizeof(T)); };
  auto closeFn = [gd]() { pirate_close(gd, O_WRONLY); };
  return Sender<T>(sendFn, closeFn);
}

template<typename T>
Sender<T> pirateSender(const std::string& config, int gd) {
    pirate_channel_param_t param;

    if (pirate_parse_channel_param(config.c_str(), &param) < 0) {
        channel_errlog([config](FILE* f) { fprintf(f, "%s unable to set channel parameter", config.c_str()); });
        exit(-1);
    }
    if (pirate_set_channel_param(gd, O_WRONLY, &param) < 0) {
        channel_errlog([config](FILE* f) { fprintf(f, "%s unable to set channel parameter", config.c_str()); });
        exit(-1);
    }
    if (pirate_open(gd, O_WRONLY) < 0) {
        channel_errlog([config](FILE* f) { fprintf(f, "Open %s failed (error = %d)", config.c_str(), errno); });
        exit(-1);
    }
    // Return sender
    return gdSender<T>(config, gd);
}

/**
 * Read messages from file descriptor.
 *
 * Note. This read is tailored to a blocking datgram interface
 * where we expect each call will read a precise number of bytes.
 */
template<typename T>
void gdDatagramReadMessages(const std::string& config, int gd, std::function<void(const T&)> f)
{
  while (true) {
    T x;
    ssize_t cnt = pirate_read(gd, &x, sizeof(T));
    if (cnt == -1) {
      channel_errlog([config](FILE* f) { fprintf(f, "Read %s failed (error = %d)", config.c_str(), errno); });
      exit(-1);
    }
    if (cnt == 0) { 
      break;
    }
    if (cnt != sizeof(T)) {
      channel_errlog([config, cnt](FILE* f) { fprintf(f, "Read %s incorrect bytes (expected = %lu, received = %lu)", config.c_str(), sizeof(T), cnt); });
      exit(-1);
    }
    f(x);
  }
  pirate_close(gd, O_RDONLY);
}

template<typename T>
Receiver<T> gdReceiver(const std::string& config, int gd) {
    return [config, gd](std::function<void (const T& d)> fn) {
      gdDatagramReadMessages<T>(config, gd, fn);
    };
}

template<typename T>
Receiver<T> pirateReceiver(const std::string& config, int gd) {
    pirate_channel_param_t param;

    if (pirate_parse_channel_param(config.c_str(), &param) < 0) {
        channel_errlog([config](FILE* f) { fprintf(f, "%s unable to set channel parameter", config.c_str()); });
        exit(-1);
    }
    if (pirate_set_channel_param(gd, O_RDONLY, &param) < 0) {
        channel_errlog([config](FILE* f) { fprintf(f, "%s unable to set channel parameter", config.c_str()); });
        exit(-1);
    }
    if (pirate_open(gd, O_RDONLY) < 0) {
        channel_errlog([config](FILE* f) { fprintf(f, "Open %s failed (error = %d)", config.c_str(), errno); });
        exit(-1);
    }
    // Return sender
    return gdReceiver<T>(config, gd);
}
