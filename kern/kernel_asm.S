
#include <kernel_asm.h>
	

DEFINE_ALL_SET_SEG

.global set_kernel_segments
set_kernel_segments:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	mov $SEGSEL_KERNEL_DS, %bx
	mov %bx, %ds
	mov %bx, %es
	mov %bx, %fs
	mov %bx, %gs
	popl %ebx
	popl %ebp
	ret
	
.global set_user_segments
set_user_segments:	
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
	mov $SEGSEL_USER_DS, %bx
	mov %bx, %ds
	mov %bx, %es
	mov %bx, %fs
	mov %bx, %gs
	popl %ebx
	popl %ebp
	ret
	
/*
 * Exit point for all newly created
 * kernel threads
 */
.global exit_kernel
exit_kernel:
	SCW_EPILOG
	
.global exit_exec
exit_exec:
	pushl %ebp
	movl %esp, %ebp
	call sched_save_kernel_esp0
	call unlock_kernel
	// Restore page directory
	movl $EX_PAGEDIR_OFFSET, %ebx 
	pushl (%ebp, %ebx, ARG_DWORD_SIZE)
	call vmm_set_user_pgdir
	popl %edx
	call set_user_segments
	movl $SEGSEL_USER_DS, %ebx
	pushl %ebx
	movl $EX_ESP_OFFSET, %ebx
	pushl (%ebp, %ebx, ARG_DWORD_SIZE)
	movl $EX_EFLAGS_OFFSET, %ebx
	pushl (%ebp, %ebx, ARG_DWORD_SIZE)
	movl $SEGSEL_USER_CS, %ebx
	pushl %ebx
	movl $EX_EIP_OFFSET, %ebx
	pushl (%ebp, %ebx, ARG_DWORD_SIZE)
	iret

