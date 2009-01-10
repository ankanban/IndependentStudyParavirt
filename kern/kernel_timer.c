
#include <timer.h>
#include <sched.h>

unsigned int numTicks;
unsigned int numSeconds;

void
kernel_tick(unsigned int  numTick)
{
  if (numTicks % TIMER_INTERRUPT_HZ == 0){
    numSeconds++;
  }

  numTicks = numTick;

  return;
}

unsigned int
kernel_getticks()
{
  return numTicks;
}

unsigned int
kernel_getseconds()
{
  return numSeconds;
}
