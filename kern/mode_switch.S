#include <x86/cr.h>
#include <mode_switch.h>
		
.global do_mode_switch

do_mode_switch:
	pushl %ebp
	// Set the frame pointer
        movl %esp, %ebp
	
	// SS
	movl $MS_SS_OFFSET, %ebx
	movl (%ebp, %ebx, ARG_DWORD_SIZE), %eax			
	pushl %eax

	// ESP
	movl $MS_ESP_OFFSET, %ebx
	movl (%ebp, %ebx, ARG_DWORD_SIZE), %eax
	pushl %eax
	
	// EFL
	movl $MS_EFLAGS_OFFSET, %ebx
	movl (%ebp, %ebx, ARG_DWORD_SIZE), %eax
	pushl %eax

	// CS
	movl $MS_CS_OFFSET, %ebx
	movl (%ebp, %ebx, ARG_DWORD_SIZE), %eax
	pushl %eax

	
	// EIP
	movl $MS_EIP_OFFSET, %ebx
	movl (%ebp, %ebx, ARG_DWORD_SIZE), %eax
	pushl %eax
	
	// DS
	movl $MS_DS_OFFSET, %ebx
	movl (%ebp, %ebx, ARG_DWORD_SIZE), %ecx
	
	mov %cx, %ds
	mov %cx, %es
	mov %cx, %fs
	mov %cx, %gs

	// Zero out gen-purpose registers
	xor %eax, %eax
	xor %ebx, %ebx
	xor %ecx, %ecx
	xor %edx, %edx
	xor %edi, %edi
	xor %esi, %esi
	
	// Switch to user-mode
	iret
