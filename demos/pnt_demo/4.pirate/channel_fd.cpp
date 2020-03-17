#include "channel_fd.h"

#include "libpirate.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void gdCheckedWrite(const std::string& path, int gd, const void* buf, size_t n) {
  ssize_t cnt = pirate_write(gd, buf, n);
  if (cnt == -1) {
    channel_errlog([path](FILE* f) { fprintf(f, "%s write failed (error = %d).", path.c_str(), errno); });
    exit(-1);
  }
  if (cnt < n) {
    channel_errlog([path, cnt](FILE* f) { fprintf(f, "%s incomplete write (bytes = %zd).", path.c_str(), cnt); });
    exit(-1);
  }
}

void piratePipe(const std::string& path, int gd) {
  int rv;
  pirate_channel_param_t param;

  pirate_init_channel_param(PIPE, &param);
  strncpy(param.channel.pipe.path, path.c_str(), sizeof(param.channel.pipe.path));
  if (pirate_set_channel_param(gd, O_RDONLY, &param) < 0) {
    channel_errlog([](FILE* f) { fprintf(f, "unable to set read channel parameter"); });
    exit(-1);
  }
  if (pirate_set_channel_param(gd, O_WRONLY, &param) < 0) {
    channel_errlog([](FILE* f) { fprintf(f, "unable to set write channel parameter"); });
    exit(-1);
  }
  if (pirate_pipe(gd, O_RDWR) < 0) {
    channel_errlog([](FILE* f) { fprintf(f, "unable to open pirate pipe"); });
    exit(-1);
  }
}
