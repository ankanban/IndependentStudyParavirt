/* Test program for 15-410 project 3 Spring 2004
 * Michael Ashley-Rollman(mpa)
 *
 * tests the basic functionality of remove_pages
 * remove_pages_test2.c
 */

#include <syscall.h>
#include <stdio.h>
#include "410_tests.h"

#define ADDR 0x40000000
#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

static char test_name[] = "remove_pages_test2:";

int main() {
  lprintf("%s%s%s", TEST_PFX, test_name, TEST_START_ABORT);
  lprintf("This test checks that remove_pages removed the pages\n");
  lprintf("It assumes that new_pages works as spec\n");

  /* allocate a couple pages */
  if (new_pages((void *)ADDR, 5 * PAGE_SIZE) != 0) {
    lprintf("new_pages failed at %x", ADDR);
    lprintf("%s%s%s", TEST_PFX, test_name, TEST_END_FAIL);
    return -1;
  }

  if (*(int *)(ADDR + 2 * PAGE_SIZE) != 0) {
    lprintf("allocated page not zeroed at %x", ADDR + 2 * PAGE_SIZE);
    lprintf("%s%s%s", TEST_PFX, test_name, TEST_END_FAIL);
    return -1;
  }
  *(int *)(ADDR + 2 * PAGE_SIZE) = 5;

  if (remove_pages((void *)ADDR) != 0) {
    lprintf("remove_pages failed to remove pages at %x", ADDR);
    lprintf("%s%s%s", TEST_PFX, test_name, TEST_END_FAIL);
    return -1;
  }  

  lprintf("%s%s%s", TEST_PFX, test_name, "pages removed");
    
  switch (*(int *)(ADDR + 2 * PAGE_SIZE)) {
  case 5:
    lprintf("%s%s%s", TEST_PFX, test_name, "pages not really removed");
    break;

  default:
    lprintf("%s%s%s%d%s", TEST_PFX, test_name, "memory still mapped, but got",
	    *(int *)(ADDR + 2 * PAGE_SIZE), " instead of 5");
    break;
  }
    lprintf("%s%s%s", TEST_PFX, test_name, TEST_END_FAIL);
    return -1;
}
