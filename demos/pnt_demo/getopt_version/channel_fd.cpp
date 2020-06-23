#include "channel_fd.h"

#include "libpirate.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void gdCheckedWrite(int gd, const void* buf, size_t n) {
  ssize_t cnt = pirate_write(gd, buf, n);
  if (cnt == -1) {
    char config[128];
    pirate_get_channel_description(gd, config, sizeof(config));
    channel_errlog([config](FILE* f) { fprintf(f, "%s write failed (error = %d).", config, errno); });
    exit(-1);
  }
  if (cnt < n) {
    char config[128];
    pirate_get_channel_description(gd, config, sizeof(config));
    channel_errlog([config, cnt](FILE* f) { fprintf(f, "%s incomplete write (bytes = %zd).", config, cnt); });
    exit(-1);
  }
}
