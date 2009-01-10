
.global gettid_wrapper

gettid_wrapper:
	/* Save callee save registers */
	pushl %ebp
	movl %esp, %ebp	
	pushl %ebx
	pushl %edi
	pushl %esi
	
	call sys_gettid

	popl %esi
	popl %edi
	popl %ebx
	popl %ebp
	
	/* Return value is in eax */
	iret
