/* Test program for 15-410 project 3 Fall 2003
 * Michael Ashley-Rollman(mpa)
 *
 * tests the basic functionality of readline
 * readline_basic.c
 */

#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <simics.h>
#include "410_tests.h"

static char test_name[] = "readline_basic:";

int main()
{
  char buf[100];
  lprintf("%s%s%s",TEST_PFX,test_name,TEST_START_CMPLT);

  if (magic_readline(100, buf) < 0)
    lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_FAIL);

  lprintf("%s%s%s",TEST_PFX,test_name,buf);

  lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_SUCCESS);
  exit(0);
}
