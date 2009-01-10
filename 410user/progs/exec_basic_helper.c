/* Test program for 15-410 project 3 Fall 2003
 * Michael Ashley-Rollman(mpa)
 *
 * program to be called by exec_basic
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
  REPORT_MISC("exec_basic_helper main() starting...");
  REPORT_END_SUCCESS;
 
  exit(1);
}
