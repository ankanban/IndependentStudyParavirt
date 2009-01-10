#include <simics.h>

int gettid(void);

void functest()
{
  lprintf("Tested func calls");
}

void
task_idle()
{
  int tid = 0;

  functest();

  //MAGIC_BREAK;
  tid = gettid();

  lprintf("TID: %d", tid);

  while (1) {
    continue;
  }
}
