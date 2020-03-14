#include "channel_fd.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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
        channel_error([path](std::ostream& o) { o << "Could not open " << path << " (error = " << errno << ").\n"; });
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
