/**
  * Test from Fall 03
  * Much thanks to Raza Ali & Kevin Caffrey
  */

#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "410_tests.h"

DEF_TEST_NAME("make_crash:");

int main(int argc, char **argv) {
	if (argc < 3) {
		return -50;
	}
	char buf[200];
	sprintf(buf, "   My program is %s, my argument is %s and %s TID=%d", argv[0], argv[1], argv[2], gettid());
	REPORT_MISC(buf);

	if (fork()==0)
		return 0;

	REPORT_MISC("   I am an exec-ed parent");

  /* Should print when parent execs and exits */
	if (gettid() == atoi(argv[2])) {
		REPORT_END_SUCCESS;
		return 10000;
	}

	return gettid(); 
}
