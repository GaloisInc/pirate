#include "print.h"
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void print(std::function<void(std::ostream& o)> p) {
  std::stringstream o;
  p(o);
  std::string s=o.str();
  write(STDOUT_FILENO, s.c_str(), s.size());
}

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
  fclose(f);
  write(STDERR_FILENO, b, sz);
  free(b);
}

void channel_error(std::function<void(std::ostream& o)> p) {
  std::stringstream o;
  p(o);
  std::string s=o.str();
  write(STDERR_FILENO, s.c_str(), s.size());

  exit(-1);
}
