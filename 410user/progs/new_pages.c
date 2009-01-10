/* Test program for 15-410 project 3 Spring 2004
 * Dave Eckhardt
 *
 * beat on new_pages() some
 * new_pages.c
 */

/* Here's what we do */
int new_stack(void);    /* new_pages() into the stack */
int new_data(void);     /* new_pages() into data region */
int lotsa_luck(void);   /* new_pages() "too much" memory */

#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "410_tests.h"

/* Yes, I mean this--see below */
#define TEST_NAME "new_pages:"
static char test_name[] = TEST_NAME;

#include "npgs_exhaustion.i"

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif
#define PAGE_BASE(a) (((int)(a)) & ~(PAGE_SIZE - 1))

int
main(int argc, char *argv[])
{
	int rets[3], i;
	char buf[50];

	REPORT_START_CMPLT;

	rets[0] = new_stack();
	rets[1] = new_data();
	rets[2] = lotsa_luck();

	for (i = 0; i < sizeof (rets) / sizeof (rets[0]); ++i) {
		if (rets[i] == 0) {
			REPORT_MISC("I *want* my outs to count!!!"); /* Christine Lavin */
			sprintf(buf, "Died on %d", i);
			REPORT_MISC(buf);
			REPORT_END_FAIL;
			exit(71);
		}
	}

	exhaustion();

	REPORT_END_SUCCESS;
	exit(0);
}

int
new_stack(void)
{
	int answer = 42;
	int ret;

	ret = new_pages((void *)PAGE_BASE((&answer)), PAGE_SIZE);
	if (answer != 42) {
		REPORT_MISC("My brain hurts!"); /* T.F. Gumby */
		REPORT_END_FAIL;
		exit(71);
	}
	return (ret);
}

int
new_data(void)
{
	int ret;

	ret = new_pages((void *)PAGE_BASE(test_name), PAGE_SIZE);
	if (strcmp(test_name, TEST_NAME)) {
		lprintf("%s%s%s",TEST_PFX,TEST_NAME,"My name is... my name is..."); /* eminem */
		REPORT_END_FAIL;
		exit(71);
	}
	return (ret);
}

#define ADDR 0x40000000
#define GIGABYTE (1024*1024*1024)

int
lotsa_luck(void)
{
	return (new_pages((void *)ADDR, GIGABYTE));
}
