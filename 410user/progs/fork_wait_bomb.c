#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include "410_tests.h"
static char test_name[]= "fork_wait_bomb:";

int main(int argc, char *argv[])
{
	int pid = 0;
  int count = 0;
	int ret_val;
	int wpid;

	lprintf("%s%s%s",TEST_PFX,test_name,TEST_START_CMPLT); 

	lprintf("parent pid: %d", gettid());

	while(count < 1000) {
		if((pid = fork()) == 0) {
			exit(42);
		}
		if(pid < 0) {
			break;
		}

    count++;

		lprintf("%s%s child pid: %d", TEST_PFX,test_name,pid); /* progress indicator */

		wpid = wait(&ret_val);

		if(wpid != pid ||
			 ret_val != 42) {
		  lprintf("error");
		  lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_FAIL);
		}
	}

	lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_SUCCESS);
	exit(42);
}
