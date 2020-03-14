#pragma once
#include "channel.h"
#include "print.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

template<typename T>
Sender<T> fdSender(int fd) {
  return [fd](const T& d) {
      ssize_t cnt = write(fd, &d, sizeof(T));
      if (cnt == -1) {
          channel_error([fd](std::ostream& o) { o << "Write " << fd << " failed: " << errno << std::endl; });
      }
      if (cnt < sizeof(T)) {
          channel_error([fd](std::ostream& o) {
              o << "Write " << fd << " fewer bytes than expected." << std::endl;
            });
      }
    };
}

template<typename T>
void fdReadMessages(int fd, std::function<void (const T& d)> f)
{
  char bytes[sizeof(T)];
  int off = 0;
  while (true) {
    T p;
    ssize_t cnt = read(fd, bytes + off, sizeof(T) - off);
    if (cnt == -1) {
      channel_error([fd](std::ostream& o) { o << "Read " << fd << " failed: " << errno << std::endl; });
    }
    if (cnt == 0) {
      channel_error([fd](std::ostream& o) { o << "Read " << fd << " expected non-zero bytes." << std::endl; });
    }
    if (cnt == (sizeof(T) - off)) {
      f(*reinterpret_cast<T*>(bytes));
    } else {
      off += cnt;
    }
  }
}

template<typename T>
Receiver<T> fdReceiver(int fd)
{
  return [fd](std::function<void (const T& d)> f) { fdReadMessages<T>(fd, f); };
}

/** Create a sender receiver pair `pipe` */
template<typename T>
SenderReceiverPair<T> anonPipe()
{
  int fd[2];
  if (pipe(fd)) { std::cerr << "pipe failed" << std::endl; exit(-1); }
  return { .sender = fdSender<T>(fd[1]),
           .receiver = fdReceiver<T>(fd[0])
         };
}

int openFifo(const std::string& path, int flags);

template<typename T>
Sender<T> fifoSender(const std::string& path) {
    return fdSender<T>(openFifo(path, O_WRONLY | O_DSYNC | O_SYNC));
}

template<typename T>
Receiver<T> fifoReceiver(const std::string& path) {
    return fdReceiver<T>(openFifo(path, O_RDONLY | O_RSYNC | O_SYNC));
}

inline
int mkUnixSocket(const std::string& path)
{
    int fd = socket(AF_UNIX, SOCK_DGRAM, 0);

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path)-1);
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        channel_error([](std::ostream& o) { o << "Bind failed: " << errno << std::endl; });
    }
    return fd;
}

template<typename T>
Sender<T> unixPipeSender(const std::string& path) {
    return fdSender<T>(mkUnixSocket(path));
}

template<typename T>
Receiver<T> unixPipeReceiver(const std::string& path) {
    return fdReceiver<T>(mkUnixSocket(path));
}