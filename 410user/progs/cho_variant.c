/** Test program for 15-410 project 3 Fall 2005
 * Dave Eckhardt (modified by ctokar and nwf)
 *
 * "cho" = "continuous hours of operation"
 *
 * keep a pile of tests of specifiled names active
 *
 * Added by nwf:
 *  - Check that we matched pid given by wait to a process we spawned;
 *    fail if that's not the case.
 *  - Check return codes from programs against expectation, fail if
 *    mismatch.
 */

#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "410_tests.h"
#include <simics.h>

static char test_name[] = "cho_variant:";

/** @brief Every UPDATE_FREQUENCY reaps, print out an update of 
 * how many children of each type are left.
 */
static const int UPDATE_FREQUENCY = 10;

/** @brief Error signaling return code.
 * @pre Ensure that no test program in the table below returns this
 * constant.
 */
static const int DEAD_TEST_PROGRAM_RETURN = 0xDEADDEAD;

/** @brief Program table */
struct prog {
  char *name;
  int expected;
  int pid;
  int count;
} progs[]
  = {
  {"exec_basic", 1, -1, 5},
  {"fork_wait", 0, -1, 5},
  {"loader_test1", 0, -1, 5},
  {"loader_test2", 42, -1, 5},
  {"make_crash",10000,-1, 1},
  {"print_basic",0,-1,5},
  {"remove_pages_test1", 0, -1, 2},
  {"remove_pages_test2", -2, -1, 5}, /* Kernel kills with -2 */
  {"sleep_test1", 42, -1, 5},  /* indistinguishible outcomes */
  {"stack_test1", 42, -1, 5},
  {"wild_test1", -2, -1, 5},   /* Kernel kills with -2 */
  {"yield_desc_mkrun", 0, -1, 5},
};

int main(int argc, char **argv)
{
  int active_progs = sizeof (progs) / sizeof (progs[0]);
  int active_processes = 0;
  struct prog *p, *fence;
  int reap_count = 0;
  int matched;
  int magic_break = 0;

  if ( argc > 1 )
  {
    if ( !strcmp("MAGIC", argv[1]) )
    {
      magic_break = 1;
    }
  }

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

           lprintf(
            "%s%s After fork(): I am a child (prog = %s, tid = %d)!",
            TEST_PFX, test_name, p->name, gettid() );

          args[0] = p->name;
          args[1] = 0;
          exec(p->name, args);
          lprintf("%s%s%s", TEST_PFX,test_name, " exec FAILED");
          /* Leave it to the parent to emit the test failure message */
          if ( magic_break )
          {
            MAGIC_BREAK;
          }
          exit(DEAD_TEST_PROGRAM_RETURN);
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
      if (status == DEAD_TEST_PROGRAM_RETURN)
      {
        lprintf("%s%s%s", TEST_PFX,test_name,TEST_END_FAIL);
        if ( magic_break )
        {
          MAGIC_BREAK;
        }
        exit(-1);
      }
      ++reap_count;
      for (p = progs, matched = 0; p < fence; ++p) {
        if (p->pid == pid) {
          matched = 1;
          lprintf("%s%s After wait(): pid = %d, prog = %s", 
                        TEST_PFX, test_name, pid, p->name);
          if ( status != p->expected )
          {
            lprintf("=( TEST FAILED: %s; EXPECT %d GOT %d",
                    p->name, p->expected, status );
            lprintf("%s%s%s", TEST_PFX,test_name,TEST_END_FAIL);
            if ( magic_break )
            {
              MAGIC_BREAK;
            }
            exit(-1);
          }
          p->pid = -1;
          --p->count;
          if (p->count <= 0)
            --active_progs;
          --active_processes;
        }
        if (reap_count % UPDATE_FREQUENCY == 0)
        {
          /* Display progress on softcons as well, in condensed form */
          lprintf("%s%s %s: %d left", TEST_PFX,test_name, p->name, p->count);
          printf("[%s,%d]",p->name, p->count);
        }
      }
      if ( matched == 0 )
      {
        lprintf(
          "=( WAIT RETURUNED SOMETHING (%d; code %d) I DIDN'T LAUNCH!",
          pid, status);
        lprintf("%s%s%s", TEST_PFX,test_name,TEST_END_FAIL);
        if ( magic_break )
        {
          MAGIC_BREAK;
        }
        exit(-1);
      }
    }
    if (reap_count % UPDATE_FREQUENCY == 0)
      print(1, "\n");
  }

  lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_SUCCESS);
  exit(0);
}
