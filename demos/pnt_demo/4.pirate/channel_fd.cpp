#include "channel_fd.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void fdCheckedWrite(const std::string& path, int fd, const void* buf, size_t n) {
  ssize_t cnt = write(fd, buf, n);
  if (cnt == -1) {
    channel_errlog([path](FILE* f) { fprintf(f, "%s write failed (error = %d).", path.c_str(), errno); });
    exit(-1);
  }
  if (cnt < n) {
    channel_errlog([path, cnt](FILE* f) { fprintf(f, "%s incomplete write (bytes = %zd).", path.c_str(), cnt); });
    exit(-1);
  }
}

int openFifo(const std::string& path, int flags) {
    if (mkfifo(path.c_str(), 0660) == -1) {
      if (errno == EEXIST) {
        errno = 0;
      } else {
        channel_errlog([path](FILE* f) { fprintf(f, "%s creation failed (error = %d).", path.c_str(), errno); });
        exit(-1);
      }
    }

    int fd = open(path.c_str(), flags);
    if (fd == -1) {
        channel_errlog([path](FILE* f) { fprintf(f, "%s open failed (error = %d).", path.c_str(), errno); });
        exit(-1);
    }

    struct stat s;
    if (fstat(fd, &s) == -1) {
        channel_errlog([path](FILE* f) { fprintf(f, "%s stat failed (error = %d).", path.c_str(), errno); });
        exit(-1);
    }
    if (!S_ISFIFO(s.st_mode)) {
        channel_errlog([path](FILE* f) { fprintf(f, "%s must be a fifo.", path.c_str()); });
        exit(-1);
    }
    return fd;
}
