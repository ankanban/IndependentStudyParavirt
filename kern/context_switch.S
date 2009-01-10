#include <x86/cr.h>
#include <kernel_asm.h>

#define CS_OLDTHR_OFFSET 2
#define CS_ESP_OFFSET 3
#define CS_PAGEDIR_OFFSET 4
	
.global do_context_switch

do_context_switch:
	// Set the frame pointer,
	// Save all registers on the stack.
	pushl %ebp	
        movl %esp, %ebp
	
	pushl %eax
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushl %edi
	pushl %esi

	// Pass current ESP and Old_thread ptr as
	// args to save routine.
	pushl %esp
	
	movl $CS_OLDTHR_OFFSET, %ebx 
	movl (%ebp, %ebx, ARG_DWORD_SIZE), %ecx
	pushl %ecx
	
	// Save the kernel stack pointer
	call sched_save_thread_kernel_state

	popl %ecx
	popl %esp
	
	
	// Restore page directory
	movl $CS_PAGEDIR_OFFSET, %ebx 
	pushl (%ebp, %ebx, ARG_DWORD_SIZE)

	call vmm_set_user_pgdir
	popl %ebx
	
	// Restore stack
	movl $CS_ESP_OFFSET, %ebx 
	movl (%ebp, %ebx, ARG_DWORD_SIZE), %esp
	
	
	// Restore registers
	popl %esi
	popl %edi
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax
	
	popl %ebp

	ret
