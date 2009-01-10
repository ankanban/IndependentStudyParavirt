#include <syscall.h>  /* gettid() */
#include <stdio.h>
#include <simics.h>   /* lprintf() */

/* Main */
int main()
{
	int tid;

	asm( "pushl %%ebx;"
		"movl $0xabadd00d, %%ebx;"
		"xchg %%ebx, %%ebx;"
		"popl %%ebx" : : );

	tid = gettid();

	lprintf("my tid is: %d", tid);
	
	while (1)
		continue;
}
