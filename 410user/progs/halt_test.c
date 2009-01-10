/* Test program for 15-410 project 3 Fall 2003
 * Michael Ashley-Rollman(mpa)
 *
 * tests the basic functionality of halt
 * halt_test.c
 */

#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <simics.h>
#include "410_tests.h"

DEF_TEST_NAME("halt_test:");

int main()
{
  REPORT_START_ABORT;

  halt();

  REPORT_END_FAIL;
  exit(-1);
}
