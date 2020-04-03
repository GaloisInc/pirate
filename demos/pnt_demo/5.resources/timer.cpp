#include "timer.h"

#include <thread>
#include <unistd.h>

#include "print.h"

void onTimer(TimerMsec start, 
             std::chrono::milliseconds dur,
             std::chrono::milliseconds step, 
             std::function<void(TimerMsec)> p)
{
  if (dur == std::chrono::milliseconds::zero()) {
    auto now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());
    while (1)
    {
      p(now);
      usleep(step.count() * 1000);
      now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());
    }
  } else {  
    TimerMsec now = start;
    TimerMsec total = now + dur;
    while (now < total)
    {
      p(now);
      usleep(step.count() * 1000);
      now += step;
    }
  }
}