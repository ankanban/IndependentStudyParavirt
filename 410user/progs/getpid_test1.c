/* Test program for 15-412 project 3 Spring 2003
 * Zach Anderson(zra)
 *
 * a getpid() test
 * getpid_test1.c
 * spits the pid of this process out 
 * onto the simics console.
 */

/* Includes */
#include <syscall.h>  /* for getpid */
#include <stdlib.h>   /* for exit */
#include <simics.h>    /* for lprintf */
#include "410_tests.h"
static char test_name[]= "getpid_test1:";

/* Main */
int main()
{
	int pid;

	lprintf("%s%s%s",TEST_PFX,test_name,TEST_START_CMPLT);	

	pid = gettid();

	lprintf("%s%s my pid is: %d",TEST_PFX,test_name, pid );
	
	if(pid == gettid()) {
	  lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_SUCCESS);
	}
	else {
	  lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_FAIL);
	}

	exit( 0 );
}
