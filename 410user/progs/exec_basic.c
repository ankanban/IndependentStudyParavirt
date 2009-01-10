/* Test program for 15-410 project 3 Fall 2003
 * Michael Ashley-Rollman(mpa)
 *
 * tests the basic functionality of exec
 * exec_basic.c
 */

#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <simics.h>
#include "410_tests.h"

DEF_TEST_NAME("exec_basic:");

int main()
{

  REPORT_LOCAL_INIT;  

  char *name = "exec_basic_helper";
  char *args[] = {name, 0};

  REPORT_START_CMPLT;

  REPORT_ON_ERR(exec(name,args));

	REPORT_END_FAIL;

  exit(-1);
}
