/* Test program for 15-410 project 3 Fall 2003
 * Dave Eckhardt (modified by ctokar)
 *
 * **> Public: Yes
 * **> Covers: fork,exec,solidity
 * **> NeedsWork: Yes
 * **> For: P3
 * **> Authors: de0u,ctokar
 * **> Notes: Needs progress checking implemented; otherwise alright.
 *
 * "cho" = "continuous hours of operation"
 *
 * keep a pile of tests of specifiled names active
 */

#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include "410_tests.h"
#include <simics.h>

static char test_name[] = "cho2:";

/* Every UPDATE_FREQUENCY reaps, print out an update of 
 * how many children of each type are left.
 */
#define UPDATE_FREQUENCY 50

struct prog {
	char *name;
	int pid;
	int count;
} progs[] = {
	{"getpid_test1", -1, 13},
	{"yield_desc_mkrun", -1, 100},
	{"remove_pages_test1", -1, 100},
	{"loader_test1", -1, 100},
	{"fork_wait", -1, 100}
};

int main()
{
	int active_progs = sizeof (progs) / sizeof (progs[0]);
	int active_processes = 0;
	struct prog *p, *fence;
	int reap_count = 0;

	lprintf("%s%s%s",TEST_PFX,test_name,TEST_START_CMPLT);

	fence = progs + active_progs;

	while ((active_progs > 0) || (active_processes > 0)) {

		/* launch some processes */
		for (p = progs; p < fence; ++p) {
			if ((p->count > 0) && (p->pid == -1)) {
				int pid = fork();

				if (pid == -1) {
					sleep(1);
				} else if (pid == 0) {
					char *args[2];

	 			        lprintf("%s%s After fork(): I am a child!", TEST_PFX, test_name);

					args[0] = p->name;
					args[1] = 0;
					exec(p->name, args);
					lprintf("%s%s%s", TEST_PFX,test_name, " exec FAILED (missing object?)");
					lprintf("%s%s%s", TEST_PFX,test_name,TEST_END_FAIL);
					exit(0xdeadbeef);
				} else {
					p->pid = pid;
					++active_processes;
				}
			}
		}

		/* reap a child */
		if (active_processes > 0) {
			int pid, status;
			pid = wait(&status);
			if (status == 0xdeadbeef)
			  exit(0);
			lprintf("%s%s After wait(): pid = %d", TEST_PFX, test_name, pid); /* progress */
			++reap_count;
			for (p = progs; p < fence; ++p) {
				if (p->pid == pid) {
					p->pid = -1;
					--p->count;
					if (p->count <= 0)
						--active_progs;
					--active_processes;
				}
				if (reap_count % UPDATE_FREQUENCY == 0)
				  lprintf("%s%s %s: %d left", TEST_PFX,test_name, p->name, p->count);
			}
		}
	}

	lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_SUCCESS);
	exit(0);
}
