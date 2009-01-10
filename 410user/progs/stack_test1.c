/*
 * stack test 1
 */

#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include "410_tests.h"
static char test_name[]= "stack_test1:";

int first = 0;
int last = 0;
int num_non_zero = 0;

void foo(void)
{
  int i;
  char c[65536];

  for (i = 65535; i >= 0; --i)
    if (c[i] != 0) {
      num_non_zero++;
      if (num_non_zero == 1)
	first = i;
      last = i;
    }
  return;
}

int main()
{
  foo();

  lprintf("%s%s%s",TEST_PFX,test_name,TEST_START_CMPLT);  

  if (num_non_zero != 0) {
    lprintf("NB: Since the stack grows down and arrays grow up, i=65535\n\
    corresponds to somewhere in the middle of the stack while i=0 is the\n\
    top (or lowest value in memory since they grow down...)\n");
    lprintf("%d nonzero bytes in memory", num_non_zero);
    lprintf("first nonzero memory value at i=%d", first);
    lprintf("last nonzero memory value at i=%d", last);
    lprintf("thus, %d of first %d bytes on stack were nonzero\n", num_non_zero, 65535-last);

    lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_FAIL);  
    exit(-1);
  }

  lprintf("%s%s%s",TEST_PFX,test_name,"stack allocation successful.");
  lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_SUCCESS);  
  exit(42);
}
