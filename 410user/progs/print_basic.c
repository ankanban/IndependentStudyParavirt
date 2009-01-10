/* Test program for 15-410 project 3 Fall 2003
 * Michael Ashley-Rollman(mpa)
 *
 * @author Updated 4/1/04 Mark T. Tomczak (mtomczak)
 *
 * tests the basic functionality of print
 * print_basic.c
 */

#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include "410_tests.h"

DEF_TEST_NAME("print_basic:");

int main()
{
  REPORT_START_CMPLT;
  if (print(13, "Hello World!\n") != 0)
    REPORT_END_FAIL;
  else
    REPORT_END_SUCCESS;

  exit(0);
}
