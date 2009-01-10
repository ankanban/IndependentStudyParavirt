/* Test program for 15-412 project 3 Spring 2003
 * Zach Anderson(zra)
 *
 * testing robustness of kernel.
 * fork_bomb.c
 * A fork bomb.
 * Kernel should gracefully handle running out of
 * memory.
 */

/* Includes */
#include <syscall.h>    /* for fork */
#include <stdio.h>      /* for lprintf */
#include <stdlib.h>
#include "410_tests.h"
static char test_name[]= "fork_bomb:";

/* Main */
int main()
{
	lprintf("%s%s%s",TEST_PFX,test_name,TEST_START_4EVER); 

  TEST_PROG_ENGAGE(200);

	while( 1 ){
    fork();
    TEST_PROG_PROGRESS;
  }

	exit(42);
}
