/* Test program for 15-410 project 3 Fall 2003
 * Michael Ashley-Rollman(mpa)
 *
 * tests the basic functionality of exec
 * exec_basic.c
 */

#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include "410_tests.h"

static char test_name[] = "exec_nonexist:";

int main()
{
  lprintf("%s%s%s",TEST_PFX,test_name,TEST_START_CMPLT);
  char *name = "exec_a_random_file_that_does_not_exist";
  char *args[] = {name, 0};

  if (exec(name,args) >= 0) {
    lprintf("%s%s%s",TEST_PFX,test_name," exec return value not less than zero");
    lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_FAIL);
    exit(-1);
  }

  lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_SUCCESS);

  exit(0);
}
