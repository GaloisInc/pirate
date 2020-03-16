#include "timer.h"

#include <thread>
#include <unistd.h>

#include "print.h"

void onTimer(unsigned total, unsigned msec, std::function<void()> p)
{
  int cnt = 0;
  while (cnt < total)
  {
     print([](std::ostream& o) { o << "on timer" << std::endl; });
       // here we simulate sensor data streams
     p();
     print([msec](std::ostream& o) { o << "on timer done " << msec << std::endl; });
     if (usleep(msec * 1000) != 0) {
       channel_errlog([](FILE* f) { fprintf(f, "usleep failed (error = %d)", errno); });
       exit(-1);
     }
     cnt += msec;
     print([](std::ostream& o) { o << "on timer sleep resume" << std::endl; });
  }
}

/** Start a thread that runs the task at a periodic interval. */
void startTimer(unsigned total, unsigned msec, std::function<void()> p)
{
  print([](std::ostream& o) { o << "start timer" << std::endl; });

  std::thread t(onTimer, total, msec, p);
  t.detach();
}
