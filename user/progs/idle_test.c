#include <simics.h>

int gettid(void);

void functest()
{
  lprintf("Tested func calls");
}

int
main(void)
{
  int tid = 0;

  functest();

  //MAGIC_BREAK;
  tid = gettid();

  lprintf("TID: %d", tid);

  while (1) {
    lprintf("looping");
    continue;
  }
}
