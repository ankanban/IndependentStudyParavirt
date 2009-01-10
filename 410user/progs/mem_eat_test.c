#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include "410_tests.h"
static char test_name[]= "mem_eat_test:";

#define MEM_SIZE 1024*1024
#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

void foo(int index);

int main(int argc, char *argv[])
{
  lprintf("%s%s%s",TEST_PFX,test_name,TEST_START_ABORT); 

  foo(1);
	
  lprintf("%s%s%s",TEST_PFX,test_name,TEST_END_SUCCESS);
  exit(42);
}


void foo(int index)
{
	int i;
	char local_array[MEM_SIZE];

	for(i = MEM_SIZE - 1; i >= 0; i -= PAGE_SIZE) {
		local_array[i] = 'a';
	}

	lprintf("%s%s Used %d pages, %d MB", TEST_PFX,test_name,
					(index * MEM_SIZE)/PAGE_SIZE,
					(index * MEM_SIZE)/(1024*1024));

	foo(index + 1);
}
