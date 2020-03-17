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
void fdCheckedWrite(const std::string& path, int fd, const void* buf, size_t n);

/**
 * Read messages from file descriptor.
 *
 * Note. This read is tailored to a blocking stream interface
 * where we expect each call will read some number of bytes,
 * but not necessarily the full number of bytes read.
 */
template<typename T>
void fdStreamReadMessages(const std::string& path, int fd, std::function<void(const T&)> f)
{
  char bytes[sizeof(T)];
  int off = 0;
  while (true) {
    ssize_t cnt = read(fd, bytes + off, sizeof(T) - off);
    if (cnt == -1) {
      channel_errlog([path](FILE* f) { fprintf(f, "%s read failed (error = %d)", path.c_str(), errno); });
      exit(-1);
    }
    // This usually indicates the file descriptor send channel close.d
    if (cnt == 0) {
      close(fd);
      return;
    }
    if (cnt == (sizeof(T) - off)) {
      f(*reinterpret_cast<const T*>(bytes));
    } else {
      off += cnt;
    }
  }
}

template<typename T>
Sender<T> fdSender(const std::string& path, int fd) {
  auto sendFn = [path, fd](const T& d) { fdCheckedWrite(path, fd, &d, sizeof(T)); };
  auto closeFn = [fd]() { close(fd); };
  return Sender<T>(sendFn, closeFn);
}

template<typename T>
Receiver<T> fdStreamReceiver(const std::string& path, int fd)
{
  return [path, fd](std::function<void (const T& d)> f) {
      fdStreamReadMessages<T>(path, fd, f);
    };
}

/** Create a sender receiver pair `pipe` */
template<typename T>
SenderReceiverPair<T> anonPipe(const std::string& name)
{
    int fd[2];
    if (pipe(fd)) { 
        channel_errlog([name](FILE* f) { fprintf(f, "%s pipe creation failed (error = %d).", name.c_str(), errno); });
        exit(-1);
    }
    return { .sender = fdSender<T>(name, fd[1]),
             .receiver = fdStreamReceiver<T>(name, fd[0])
           };
}

int openFifo(const std::string& path, int flags);

template<typename T>
Sender<T> fifoSender(const std::string& path) {
    return fdSender<T>(path, openFifo(path, O_WRONLY | O_DSYNC | O_SYNC));
}

template<typename T>
Receiver<T> fifoReceiver(const std::string& path) {
    return fdStreamReceiver<T>(path, openFifo(path, O_RDONLY | O_RSYNC | O_SYNC));
}

/** Create a AF_UNIX socket address that points to the given path. */
static
struct sockaddr_un checkedBindUnix(const std::string& path) {

    struct sockaddr_un saddr;

    if (path.size() >= sizeof(saddr.sun_path)) {
      channel_errlog([path](FILE* f) { fprintf(f, "%s is too long for a Unix socket path.", path.c_str()); });
      exit(-1);
    }

    memset(&saddr, 0, sizeof(saddr));
    saddr.sun_family = AF_UNIX;
    strncpy(saddr.sun_path, path.c_str(), sizeof(saddr.sun_path) - 1);
    return saddr;
}

/** Create a packet to receiver from a unix socket on the given path. */
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
        } else if (errno == ECONNREFUSED) {
            channel_errlog([path](FILE* f) { fprintf(f, "%s connection refused.", path.c_str()); });
            exit(-1);
        } else {
            channel_errlog([path](FILE* f) { fprintf(f, "%s connect error %d.", path.c_str(), errno); });
            exit(-1);
        }
    }
    // Return sender
    return fdSender<T>(path, skt);
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
      channel_errlog([path](FILE* f) { fprintf(f, "Read %s failed (error = %d)", path.c_str(), errno); });
      exit(-1);
    }
    if (cnt == 0) { 
      break;
    }
    if (cnt != sizeof(T)) {
      channel_errlog([path, cnt](FILE* f) { fprintf(f, "Read %s incorrect bytes (expected = %lu, received = %lu)", path.c_str(), sizeof(T), cnt); });
      exit(-1);
    }
    f(x);
  }
  close(fd);
}


/** 
 * Create a message receiver that creates a unix SEQPACKET socket on the given path.
 */
template<typename T>
Receiver<T> unixSeqPacketReceiver(const std::string& path) {
    // Socket for listening to connection.
    int serverSocket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    // Create struct to reference path
    struct sockaddr_un addr = checkedBindUnix(path);
    // Bind server socket to path
    if (bind(serverSocket, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) != 0) {
        if (errno == EADDRINUSE) {
            channel_errlog([path](FILE* f) { fprintf(f, "%s already in use.", path.c_str()); });
            exit(-1);
        } else {
            channel_errlog([path](FILE* f) { fprintf(f, "%s bind failed (errno = %d)", path.c_str(), errno); });
            exit(-1);
        }
    }
    // Start listening on server socket.
    if (listen(serverSocket, 0) != 0) {
        channel_errlog([path](FILE* f) { fprintf(f, "%s listen failed (errno = %d)", path.c_str(), errno); });
        exit(-1);
    }
    // Accept an incoming connection from sender.
    int rskt = accept(serverSocket, 0, 0);
    if (rskt == -1) {
        channel_errlog([path](FILE* f) { fprintf(f, "%s accept failed (errno = %d)", path.c_str(), errno); });
        exit(-1);
    }
    // Close server socket and unlink path as we no longer need to listen.
    close(serverSocket);
    unlink(path.c_str());

    // Return receiver
    return [path, rskt](std::function<void (const T& d)> fn) {
      fdDatagramReadMessages<T>(path, rskt, fn);
    };
}