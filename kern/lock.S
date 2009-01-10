
#include <kernel_asm.h>

#define MUTEX_ARG_OFFSET 2

.global mutex_trylock
.global mutex_unlock
.global mutex_spinlock

/*
 * 
 *
 */
mutex_spinlock:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	pushl %ecx
	pushl %esi

	movl $MUTEX_ARG_OFFSET, %esi
	movl (%ebp, %esi, ARG_DWORD_SIZE), %ecx 
	
	movl $1, %eax 

mutex_tryagain:	
	xorl %ebx, %ebx
	lock cmpxchgl %ebx, (%ecx)
	jnz mutex_tryagain
	
	popl %esi
	popl %ecx
	popl %ebx
	popl %ebp
	ret


/**
 *
 */
mutex_trylock:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	pushl %ecx
	pushl %esi

	movl $MUTEX_ARG_OFFSET, %esi
	movl (%ebp, %esi, ARG_DWORD_SIZE), %ecx 
	
	movl $1, %eax 
	
	xorl %ebx, %ebx
	lock cmpxchgl %ebx, (%ecx)
	jz mutex_locked

	xor %eax, %eax
	
mutex_locked:	
	popl %esi
	popl %ecx
	popl %ebx
	popl %ebp
	ret


/**
 *
 */
mutex_unlock:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	pushl %ecx	
	pushl %esi

	movl $MUTEX_ARG_OFFSET, %esi
	movl (%ebp, %esi, ARG_DWORD_SIZE), %ecx
	
	movl $1, %ebx
	
	lock xchgl %ebx, (%ecx) 

	popl %esi
	popl %ecx
	popl %ebx
	popl %ebp
	ret
