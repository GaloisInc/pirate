#include "print.h"
#include <string.h>
#include <unistd.h>

void channel_errlog(std::function<void(FILE*)> p) {
  char* b;
  size_t sz;
  FILE* f = open_memstream(&b, &sz);
  if (f == 0) {
    const char* msg = "Could not open memstream.\n";
    write(STDERR_FILENO, msg, strlen(msg));
    exit(-1);
  }
  p(f);
  fputc('\n', f);
  fclose(f);
  write(STDERR_FILENO, b, sz);
  free(b);
}