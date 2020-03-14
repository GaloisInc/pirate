#include "timer.h"

#include <thread>
#include <unistd.h>

#include "print.h"

void onTimer(int msec, std::function<void()> p)
{

  while (true)
  {
     print([](std::ostream& o) { o << "on timer" << std::endl; });
       // here we simulate sensor data streams
      p();
#ifdef _WIN32
      Sleep(sleep_msec); // 100 Hz
#else
      usleep(msec * 1000);
#endif
  }
}

/** Start a thread that runs the task at a periodic interval. */
void startTimer(int msec, std::function<void()> p)
{
  print([](std::ostream& o) { o << "start timer" << std::endl; });

  std::thread t(onTimer, msec, p);
  t.detach();
}
