#include "channel_fd.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void fdCheckedWrite(int fd, const void* buf, size_t n) {
  ssize_t cnt = write(fd, buf, n);
  if (cnt == -1) {
    channel_error([fd](std::ostream& o) { o << "Write " << fd << " failed: " << errno << std::endl; });
  }
  if (cnt < n) {
    channel_error([fd](std::ostream& o) {
        o << "Write " << fd << " fewer bytes than expected." << std::endl;
      });
  }
}

int openFifo(const std::string& path, int flags) {
    if (mkfifo(path.c_str(), 0660) == -1) {
      if (errno == EEXIST) {
        errno = 0;
      } else {
        channel_error([path](std::ostream& o) { o << "Could not create " << path << " (error = " << errno << ").\n"; });
      }
    }

    int fd = open(path.c_str(), flags);
    if (fd == -1) {
        channel_errlog([path](FILE* f) { fprintf(f, "Could not open %s (error = %d)\n", path.c_str(), errno); });
        exit(-1);
    }

    struct stat s;
    if (fstat(fd, &s) == -1) {
        channel_error([path](std::ostream& o) { o << "Could not stat " << path << " (error = " << errno << ").\n"; });
    }
    if (!S_ISFIFO(s.st_mode)) {
        channel_error([path](std::ostream& o) { o << path << " must be a fifo.\n"; });
    }
    return fd;
}
