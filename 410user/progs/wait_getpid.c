/* Test program for 15-410 project 3 Fall 2003
 * Michael Ashley-Rollman(mpa)
 *
 * tests the basic functionality of getpid assuming that wait and fork work
 * wait_getpid.c
 */

#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include "410_tests.h"

static char test_name[] = "wait_getpid:";

int main()
{
  int pid, status;

  lprintf("%s%s%s",TEST_PFX,test_name,TEST_START_CMPLT);

  pid = fork();

  if (pid < 0) {
    lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_FAIL);
    exit(-1);
  }
  
  if (pid == 0) {
    pid = gettid();
    exit(pid);
    lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_FAIL);
  }
  if (wait(&status) != pid) {
    lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_FAIL);
    exit(-1);
  }

  if (status != pid) {
    lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_FAIL);
    exit(-1);
  }


  lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_SUCCESS);
  exit(0);
}
