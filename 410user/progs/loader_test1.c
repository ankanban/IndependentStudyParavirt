/*
 * loader verifier -- test 1
 * modified by nwf to return 0 on success and 42 on failure
 */

#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include "410_tests.h"
static char test_name[]= "loader_test1:";

int feed = 0xfeed;
int beef = 0xbeef;
char x[8192] = { 1, 2, 3 };
int beef2 = 0xbeef;
int fred;

int main(int argc, char** argv) {
  
  int *ip;
  int ret=0;

  lprintf("%s%s%s",TEST_PFX,test_name,TEST_START_CMPLT); 

  if (feed != 0xfeed) {
    /* fatal */
    lprintf("%s%s%s",TEST_PFX,test_name,
	    "FAILED: .data containing wrong value");
    lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_FAIL);
    ret--;
  }

  if (beef != 0xbeef) {
    /* fatal */
    lprintf("%s%s%s",TEST_PFX,test_name,
	    "FAILED: .data containing wrong value");
    lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_FAIL);
    ret--;
  }
  
  ip = &feed;
  ++ip;

  if (*ip != 0xbeef) {
    /* fatal */
    lprintf("%s%s%s",TEST_PFX,test_name,
	    "FAILED: .data out of order");
    lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_FAIL);
    ret--;
  }
  
  if (fred) {
    /* fatal */
    lprintf("%s%s%s",TEST_PFX,test_name,
	    "FAILED: .bss not zero'd");
    lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_FAIL);
    ret--;
  }

  if (beef2 != 0xbeef) {
    /* fatal */
    lprintf("%s%s%s",TEST_PFX,test_name,
	    "FAILED: .data containing wrong value");
    lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_FAIL);
    ret--;
  }

  if(ret==0)
  {
    lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_SUCCESS);
    return 0;
  }
  exit(42);
}
