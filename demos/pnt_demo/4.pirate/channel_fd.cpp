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

void piratePipe(const std::string& config, int gd) {
  pirate_channel_param_t param;

  if (pirate_parse_channel_param(config.c_str(), &param) < 0) {
    channel_errlog([config](FILE* f) { fprintf(f, "%s unable to parse channel parameter", config.c_str()); });
    exit(-1);
  }
  if (pirate_set_channel_param(gd, O_RDONLY, &param) < 0) {
    channel_errlog([config](FILE* f) { fprintf(f, "%s unable to set read channel parameter", config.c_str()); });
    exit(-1);
  }
  if (pirate_set_channel_param(gd, O_WRONLY, &param) < 0) {
    channel_errlog([config](FILE* f) { fprintf(f, "%s unable to set write channel parameter", config.c_str()); });
    exit(-1);
  }
  if (pirate_pipe(gd, O_RDWR) < 0) {
    channel_errlog([config](FILE* f) { fprintf(f, "%s unable to open pirate pipe", config.c_str()); });
    exit(-1);
  }
}
