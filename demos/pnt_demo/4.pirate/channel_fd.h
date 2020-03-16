#pragma once
#include "channel.h"
#include "print.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

/** Write the bytes to a file descriptor, and check that all bytes were written. */
void fdCheckedWrite(int fd, const void* buf, size_t n);

/**
 * Read messages from file descriptor.
 *
 * Note. This read is tailored to a blocking stream interface
 * where we expect each call will read some number of bytes,
 * but not necessarily the full number of bytes read.
 */
template<typename T>
void fdStreamReadMessages(int fd, std::function<void(const T&)> f)
{
  char bytes[sizeof(T)];
  int off = 0;
  while (true) {
    ssize_t cnt = read(fd, bytes + off, sizeof(T) - off);
    if (cnt == -1) {
      channel_error([fd](std::ostream& o) { o << "Read " << fd << " failed: " << errno << std::endl; });
    }
    if (cnt == 0) {
      channel_error([fd](std::ostream& o) { o << "Read " << fd << " expected non-zero bytes." << std::endl; });
    }
    if (cnt == (sizeof(T) - off)) {
      f(*reinterpret_cast<const T*>(bytes));
    } else {
      off += cnt;
    }
  }
}

/**
 * Read messages from file descriptor.
 *
 * Note. This read is tailored to a blocking datgram interface
 * where we expect each call will read a precise number of bytes.
 */
template<typename T>
void fdDatagramReadMessages(const std::string& path, int fd, std::function<void(const T&)> f)
{
  while (true) {
    T x;
    ssize_t cnt = read(fd, &x, sizeof(T));
    if (cnt == -1) {
      channel_errlog([path](FILE* f) { fprintf(f, "Read %s failed (error = %d)\n", path.c_str(), errno); });
      exit(-1);
    }
    if (cnt == 0) { break; }
    if (cnt != sizeof(T)) {
      channel_errlog([path, cnt](FILE* f) { fprintf(f, "Read %s incorrect bytes (expected = %lu, received = %lu)\n", path.c_str(), sizeof(T), cnt); });
      exit(-1);
    }
    f(x);
  }
}

template<typename T>
Sender<T> fdSender(int fd) {
  return [fd](const T& d) { fdCheckedWrite(fd, &d, sizeof(T)); };
}

template<typename T>
Receiver<T> fdStreamReceiver(int fd)
{
  return [fd](std::function<void (const T& d)> f) {
      fdStreamReadMessages<T>(fd, f);
    };
}

/** Create a sender receiver pair `pipe` */
template<typename T>
SenderReceiverPair<T> anonPipe()
{
  int fd[2];
  if (pipe(fd)) { std::cerr << "pipe failed" << std::endl; exit(-1); }
  return { .sender = fdSender<T>(fd[1]),
           .receiver = fdStreamReceiver<T>(fd[0])
         };
}

int openFifo(const std::string& path, int flags);

template<typename T>
Sender<T> fifoSender(const std::string& path) {
    return fdSender<T>(openFifo(path, O_WRONLY | O_DSYNC | O_SYNC));
}

template<typename T>
Receiver<T> fifoReceiver(const std::string& path) {
    return fdStreamReceiver<T>(openFifo(path, O_RDONLY | O_RSYNC | O_SYNC));
}

static
struct sockaddr_un checkedBindUnix(const std::string& path) {

    struct sockaddr_un saddr;

    if (path.size() >= sizeof(saddr.sun_path)) {
      channel_error([](std::ostream& o) { o << "path too long" << std::endl; });
    }

    memset(&saddr, 0, sizeof(saddr));
    saddr.sun_family = AF_UNIX;
    strncpy(saddr.sun_path, path.c_str(), sizeof(saddr.sun_path) - 1);
    return saddr;
}

template<typename T>
Sender<T> unixSeqPacketSender(const std::string& path) {
    int skt = socket(AF_UNIX, SOCK_SEQPACKET, 1);
    // Create struct to reference path
    struct sockaddr_un saddr = checkedBindUnix(path);
    // Connect to receiver
    while (connect(skt, reinterpret_cast<const struct sockaddr*>(&saddr), sizeof(saddr)) != 0) {
        // If socket has not yet been created.
        if (errno == ENOENT) {
            // Sleep a millisecond to see if socket is created.
            // Note. We could use inotify to avoid polling.
            usleep(1000);
        } else {
            channel_error([](std::ostream& o) { o << "connect failed " << errno << std::endl; });
        }
    }
    // Return sender
    return fdSender<T>(skt);
}

template<typename T>
Receiver<T> unixSeqPacketReceiver(const std::string& path) {
    // Socket for listening to connection.
    int serverSocket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    // Create struct to reference path
    struct sockaddr_un addr = checkedBindUnix(path);
    // Bind server socket to path
    if (bind(serverSocket, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) != 0) {
        channel_error([path](std::ostream& o) { o << "Bind " << path << " failed: " << errno << std::endl; });
    }
    // Start listening on server socket.
    if (listen(serverSocket, 0) != 0) {
        channel_error([](std::ostream& o) { o << "Listen failed: " << errno << std::endl; });
    }
    // Accept an incoming connection from sender.
    int rskt = accept(serverSocket, 0, 0);
    if (rskt == -1) {
        channel_error([](std::ostream& o) { o << "Accept failed: " << errno << std::endl; });
    }
    // Return receiver
    return [path, rskt](std::function<void (const T& d)> fn) {
      channel_errlog([](FILE* f) { fprintf(f, "uspr0\n"); });
      fdDatagramReadMessages<T>(path, rskt, fn);
    };
}