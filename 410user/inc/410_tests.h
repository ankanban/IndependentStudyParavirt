/* Test program for 15-410 project 3 Fall 2003
 * Tadashi Okoshi (slash)
 */

#ifndef _410_TESTS_H_
#define _410_TESTS_H_

#include <simics.h>    /* lprintf() */

#define TEST_START_CMPLT " START__TYPE_COMPLETE"
#define TEST_START_ABORT " START__TYPE_ABORT"
#define TEST_START_4EVER " START__TYPE_FOREVER"
  /** An obsoleted progress detection mechanism. */
#define TEST_START_4EVER_PROGRESS " START__TYPE_FOREVER_PROGRESS"

#define TEST_END_SUCCESS " END__SUCCESS"
#define TEST_END_FAIL    " END__FAIL"

/*Don't change this for now. (slash)*/
#define TEST_PFX "(^_^)_"

/* macros added by mtomczak */
/* macro assumptions: 
   1) static char test_name[] is defined (using DEF_TEST_NAME macro)
   2) In each function, REPORT_LOCAL_INIT called before REPORT_ON_ERR
*/

#define DEF_TEST_NAME(x) static char test_name[] = x
#define REPORT_START_CMPLT \
  do{lprintf("%s%s%s",TEST_PFX,test_name,TEST_START_CMPLT);}while(0)
#define REPORT_START_ABORT \
  do{lprintf("%s%s%s",TEST_PFX,test_name,TEST_START_ABORT);}while(0)
#define REPORT_START_4EVER \
  do{lprintf("%s%s%s",TEST_PFX,test_name,TEST_START_4EVER);}while(0)

#define REPORT_END_SUCCESS \
  do{lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_SUCCESS);}while(0)
#define REPORT_END_FAIL \
  do{lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_FAIL);}while(0)

#define REPORT_LOCAL_INIT int err
#define REPORT_MISC(x) \
  do{lprintf("%s%s%s",TEST_PFX,test_name,(x));}while(0)
#define REPORT_ERR(x,errcode) \
  do{lprintf("%s%s%s%d",TEST_PFX,test_name,(x),(errcode));}while(0)
#define REPORT_FAIL_ERR(x,errcode) \
  do{REPORT_ERR(x,errcode); REPORT_END_FAIL;}while(0)
#define REPORT_ON_ERR(expression) \
  do{ \
    if((err=(expression)) < 0) \
    { \
     lprintf("%s%sErr %d on line %d: `%s'",TEST_PFX,test_name,err,__LINE__, \
              #expression);\
    }}while(0)
#define REPORT_FAILOUT_ON_ERR(expression) \
do{ \
   REPORT_ON_ERR((expression));\
   if(err < 0)\
   { \
      REPORT_END_FAIL; \
      exit(err); \
   } \
}while(0)
  
/**************************************************/
/*memo*/
/*

//At the beginning of the test code.
lprintf("%s%s%s",TEST_PFX,test_name,TEST_START_CMPLT); 
 or
lprintf("%s%s%s",TEST_PFX,test_name,TEST_START_ABORT);
 or
lprintf("%s%s%s",TEST_PFX,test_name,TEST_START_4EVER);

//In the test body...
....
lprintf("%s%s%s",TEST_PFX,test_name,"foo mesg.");
lprintf("%s%s%s",TEST_PFX,test_name,"bar mesg.");
....

//At the end of the test (only START_CMPLT case)
lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_SUCCESS);
 or
lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_FAIL);

*/

#define TEST_PROG_ENGAGE(i) asm("pushl %%ebx;            \
                                 movl $0xC0FFEE00,%%ebx; \
                                 xchg %%bx,%%bx;         \
                                 popl %%ebx" : : "a"(i) )

#define TEST_PROG_PROGRESS  asm("pushl %%ebx;            \
                                 movl $0xC0FFEE01,%%ebx; \
                                 xchg %%bx,%%bx;         \
                                 popl %%ebx" : : )

#endif /* _410_TESTS_H_ */
