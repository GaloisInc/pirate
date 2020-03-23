#include "channel_fd.h"

#include "libpirate.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void gdCheckedWrite(const std::string& config, int gd, const void* buf, size_t n) {
  ssize_t cnt = pirate_write(gd, buf, n);
  if (cnt == -1) {
    channel_errlog([config](FILE* f) { fprintf(f, "%s write failed (error = %d).", config.c_str(), errno); });
    exit(-1);
  }
  if (cnt < n) {
    channel_errlog([config, cnt](FILE* f) { fprintf(f, "%s incomplete write (bytes = %zd).", config.c_str(), cnt); });
    exit(-1);
  }
}
