/* Test program for 15-410 project 3 checkpoint 2 Spring 2004
 * Michael Ashley-Rollman(mpa)
 *
 * tests the basic functionality of fork
 * and context switching
 * knife.c
 */

#include <syscall.h>
#include <stdio.h>
#include <simics.h>

#define DELAY (16*1024)

void foo() {

}

/* this function eats up a bunch of cpu cycles */
void slow() {
  int i;

  for (i = 0; i < DELAY; i++) foo();
}

int main() {

  int tid;

  if ((tid = fork()) == 0)
    while(1) {
      slow();
      lprintf("child: %d", gettid());
    }
  else
    while(1) {
      slow();
      lprintf("parent: %d\t\tchild:%d", gettid(), tid);
    }
}
