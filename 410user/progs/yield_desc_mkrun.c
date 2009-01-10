/* Test program for 15-410 project 3 Fall 2003
 * Michael Ashley-Rollman(mpa)
 *
 * tests the basic functionality of yield() and cas2i_runflag()
 * yield_desc_mkrun.c
 */

#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include "410_tests.h"

static char test_name[] = "yield_desc_mkrun:";

int main()
{
  int parenttid, childtid, error;

  lprintf("%s%s%s",TEST_PFX,test_name,TEST_START_CMPLT);

  parenttid = gettid();
  childtid = fork();
  
  if (childtid < 0) {
    lprintf("cannot fork()");
    goto fail;
  }

  if (childtid == 0) {
    /* child */
    int old, expect, new;

    /* Wait until the process deschedules itself */
    while(yield(parenttid) == 0)
    	lprintf("%s%s%s",TEST_PFX,test_name,"parent not asleep yet");
    lprintf("%s%s%s",TEST_PFX,test_name,"parent presumably asleep");

    /* Wake parent back up */
    expect = -1;
    new = 1;
    if((error = cas2i_runflag(parenttid, &old, expect, new, expect, new)) != 0) {
      lprintf("cas2i_runflag() failed %d in child", error);
      goto fail;
    }
    if(old != expect) {
      lprintf("bad runflag in child");
      goto fail;
    }
    lprintf("%s%s%s",TEST_PFX,test_name,"parent should be awake");
  } else {
    /* parent */
    int old, expect, new;

    expect = 0;
    new = -1;
    if((error = cas2i_runflag(parenttid, &old, expect, new, expect, new)) != 0) {
      lprintf("cas2i_runflag() failed %d in parent", error);
      goto fail;
    }
    if(old != expect) {
      lprintf("bad runflag in parent");
      goto fail;
    }
    /* wait() would be better, but we're not testing wait() here */
    yield(childtid);
    yield(childtid);
    yield(childtid);
    lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_SUCCESS);
  }
  
  exit(0);

fail:
  lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_FAIL);
  exit(1);
}
