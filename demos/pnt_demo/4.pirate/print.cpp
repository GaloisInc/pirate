#include "print.h"
#include <sstream>
#include <unistd.h>

// This prints out the string written to ostream in a single
// system call.
void print(std::function<void(std::ostream& o)> p) {
  std::stringstream o;
  p(o);
  std::string s=o.str();
  write(STDOUT_FILENO, s.c_str(), s.size());
}

// This prints out the string written to ostream in a single
// system call.
void channel_error(std::function<void(std::ostream& o)> p) {
  print(p);
  exit(-1);
}
