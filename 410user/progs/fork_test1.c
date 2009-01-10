/* Test program for 15-412 project 3 Spring 2003
 * Zach Anderson(zra)
 *
 * a Fork test
 * fork_test1.c
 * Tests very basic functionality of fork
 */

/* Includes */
#include <syscall.h>   /* for fork, getpid */
#include <stdlib.h>    /* for exit */
#include <stdio.h>     /* for lprintf */
#include "410_tests.h"
static char test_name[]= "fork_test1:";

/* Main */
int main(int argc, char *argv[])
{
  int pid;
  
  lprintf("%s%s%s",TEST_PFX,test_name,TEST_START_CMPLT);	

  pid = fork();
	
  if( pid < 0 ) {
    lprintf("%s%s%s",TEST_PFX,test_name, TEST_END_FAIL);
    exit(-1);
  }
	
  if( pid > 0 ) {
    lprintf("%s%s hello from parent: pid = %d",TEST_PFX,test_name, gettid() );
  }
  else {
    lprintf("%s%s hello from child: pid = %d",TEST_PFX,test_name, gettid() );
  }


  lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_SUCCESS);
  exit( 1 );
}
