/* Test program for 15-412 project 3 Spring 2003
 * Zach Anderson(zra)
 *
 * an sleep test.
 * sleep_test1.c
 * process sleeps for amount of time given
 * as argument.
 */

/* Includes */
#include <syscall.h>  /* for sleep, getpid */
#include <stdio.h>
#include <simics.h>   /* for lprintf */
#include <stdlib.h>   /* for atoi, exit */
#include "410_tests.h"
static char test_name[]= "sleep_test1:";

/* Main */
int main(int argc, char *argv[])
{
  lprintf("%s%s%s",TEST_PFX,test_name,TEST_START_CMPLT); 
  lprintf("%s%s%s",TEST_PFX,test_name, "before sleeping" );

  if (argc == 2)
	sleep( atoi( argv[1] ) );
  else
	sleep( 3 + (gettid() % 27) );

  lprintf("%s%s%s",TEST_PFX,test_name, "after sleeping" );
  lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_SUCCESS);
  exit( 42 );
}
