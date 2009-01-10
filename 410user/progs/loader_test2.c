/*
 * large code test (loader) -- test 2
 */

#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include "410_tests.h"
static char test_name[]= "loader_test2:";

void foo(void);
void bar(void);
void no44(void);

extern int zipper;

int main(int argc, char** argv) {
  lprintf("%s%s%s",TEST_PFX,test_name,TEST_START_CMPLT); 

  foo();
  bar();

  if (zipper != 45) {
    lprintf("FAILED: code overwriting data?");
    no44();
    lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_FAIL);
    exit(-1);
  }else{
    lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_SUCCESS);
    exit(42);
  }

}

void foo(void)
{
  /* print something encouraging */
  lprintf("%s%s%s",TEST_PFX,test_name,
	  "PASSED: First call");
}

void no0(void) { 
  /* fatal */ 
  lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_FAIL);
}

void no1(void) { 
  /* fatal */
  no0();
}

void no2(void) { 
  /* fatal */ 
  no1();
}

void no3(void) { 
  /* fatal */ 
  no2();
}

void no4(void) { 
  /* fatal */
  no3();
}

void no5(void) { 
  /* fatal */ 
  no4();
}

void no6(void) { 
  /* fatal */ 
  no5();
}

void no7(void) { 
  /* fatal */
  no6();
}

void no8(void) { 
  /* fatal */ 
  no7();
}

void no9(void) { 
  /* fatal */ 
  no8();
}

void no10(void) { 
  /* fatal */
  no9();
}

void no11(void) { 
  /* fatal */ 
  no10();
}

void no12(void) { 
  /* fatal */ 
  no11();
}

void no13(void) { 
  /* fatal */ 
  no12();
}

void no14(void) { 
  /* fatal */ 
  no13();
}

void no15(void) { 
  /* fatal */
  no14();
}

void no16(void) { 
  /* fatal */ 
  no15();
}

void no17(void) { 
  /* fatal */ 
  no16();
}

void no18(void) { 
  /* fatal */
  no17();
}

void no19(void) { 
  /* fatal */ 
  no18();
}

void no20(void) { 
  /* fatal */ 
  no19();
}
void no21(void) { 
  /* fatal */ 
  no20();
}

void no22(void) { 
  /* fatal */ 
  no21();
}

void no23(void) { 
  /* fatal */
  no22();
}

void no24(void) { 
  /* fatal */ 
  no23();
}

void no25(void) { 
  /* fatal */ 
  no24();
}

void no26(void) { 
  /* fatal */
  no25();
}

void no27(void) { 
  /* fatal */ 
  no26();
}

void no28(void) { 
  /* fatal */ 
  no27();
}
void no29(void) { 
  /* fatal */ 
  no28();
}

void no30(void) { 
  /* fatal */ 
  no29();
}

void no31(void) { 
  /* fatal */
  no30();
}

void no32(void) { 
  /* fatal */ 
  no31();
}

void no33(void) { 
  /* fatal */ 
  no32();
}

void no34(void) { 
  /* fatal */
  no33();
}

void no35(void) { 
  /* fatal */ 
  no34();
}

void no36(void) { 
  /* fatal */ 
  no35();
}

void no37(void) { 
  /* fatal */ 
  no36();
}

void no38(void) { 
  /* fatal */ 
  no37();
}

void no39(void) { 
  /* fatal */
  no38();
}

void no40(void) { 
  /* fatal */ 
  no39();
}

void no41(void) { 
  /* fatal */ 
  no40();
}

void no42(void) { 
  /* fatal */
  no41();
}

void no43(void) { 
  /* fatal */ 
  no42();
}

void no44(void) { 
  /* fatal */ 
  no43();
}

void bar(void)
{
  /* print something encouraging */
  lprintf("%s%s%s",TEST_PFX,test_name,
	  "PASSED: Second call");
}

int zipper=45;
