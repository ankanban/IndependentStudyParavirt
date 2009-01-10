/** @file syscall_wrapper.S
 *  @brief Wrapper for calling pebbles system calls
 *
 *
 *  @author Anshuman P.Kanetkar (apk)
 *  
 */
#include <syscall_int.h>

#include "syscall_int.h"

	
#define INTEGER_SIZE 4
#define INT_NUM_OFFSET 2
#define ARG_NUM_OFFSET 3
#define ARG_PKT_OFFSET 4
#define INT_TABLE_ENTRY_SIZE (syscall_int_entry1 - syscall_int_table)
	
.globl syscall_wrapper
.globl syscall_wrapper_0
.globl syscall_wrapper_1
.globl syscall_wrapper_2
.globl syscall_wrapper_3	

	

/**
 * Interrupt table for calling system calls
 */
syscall_int_table:
	// Call the sycall software interrupts
	int $SYSCALL_INT
	jmp syscall_wrapper_end
syscall_int_entry1:	
	int $FORK_INT
	jmp syscall_wrapper_end
	int $EXEC_INT
	jmp syscall_wrapper_end
	int $0x43 
	jmp syscall_wrapper_end
	int $WAIT_INT
	jmp syscall_wrapper_end
	int $YIELD_INT
	jmp syscall_wrapper_end
	// Need these fake entries to make table indexing work
	int $0x46 /*$DESCHEDULE_INT*/
	jmp syscall_wrapper_end
	int $0x47 /*$MAKE_RUNNABLE_INT*/
	jmp syscall_wrapper_end
	
	int $GETTID_INT
	jmp syscall_wrapper_end
	int $NEW_PAGES_INT
	jmp syscall_wrapper_end
	int $REMOVE_PAGES_INT
	jmp syscall_wrapper_end
 	int $SLEEP_INT
	jmp syscall_wrapper_end
	int $GETCHAR_INT
	jmp syscall_wrapper_end
	int $READLINE_INT
	jmp syscall_wrapper_end
	int $PRINT_INT
	jmp syscall_wrapper_end
	int $SET_TERM_COLOR_INT
	jmp syscall_wrapper_end
	int $SET_CURSOR_POS_INT
	jmp syscall_wrapper_end
	int $GET_CURSOR_POS_INT
	jmp syscall_wrapper_end
	int $THREAD_FORK_INT
	jmp syscall_wrapper_end
	int $GET_TICKS_INT
	jmp syscall_wrapper_end
	int $MISBEHAVE_INT
	jmp syscall_wrapper_end
	int $HALT_INT
	jmp syscall_wrapper_end
	int $LS_INT
	jmp syscall_wrapper_end
	int $TASK_VANISH_INT      /* previously known as TASK_EXIT_INT */
	jmp syscall_wrapper_end
	// Fake entry
	int $0x58
	jmp syscall_wrapper_end
	int $SET_STATUS_INT
	jmp syscall_wrapper_end
	// Fake entries, to ensure table addressing works.
	int $90
	jmp syscall_wrapper_end
	int $91
	jmp syscall_wrapper_end
	int $92
	jmp syscall_wrapper_end
	int $93
	jmp syscall_wrapper_end
	int $94
	jmp syscall_wrapper_end
	int $95
	jmp syscall_wrapper_end
	int $VANISH_INT
	jmp syscall_wrapper_end
	int $CAS2I_RUNFLAG_INT
	jmp syscall_wrapper_end

	
/**
 * @brief The wrapper function
 * @param int_num Interrupt number
 * @param arg_num Number of arguments
 * @param Argument list (optional)
 */
syscall_wrapper_0:
syscall_wrapper_1:
syscall_wrapper_2:
syscall_wrapper_3:	
syscall_wrapper:
	/* Save the callee save registers */
	pushl	%ebp		
	movl	%esp,%ebp	
	pushl	%edi
	pushl	%ebx
	pushl	%esi
	
	// Get the interrupt number
	movl $INT_NUM_OFFSET, %ebx 
	movl (%ebp, %ebx, INTEGER_SIZE), %ecx

	// Get the number of arguments	
	movl $ARG_NUM_OFFSET, %ebx 				
	movl (%ebp, %ebx, INTEGER_SIZE), %edx
	
	movl $ARG_PKT_OFFSET, %ebx
	// Check if single or multiple arguments

	cmpl $0x1, %edx
	jz syscall_1arg
	cmpl $0x0, %edx
	jz syscall_0arg
	// Multiple arguments, get arg pkt offset into eax
	
 	// move pkt pointer into %esi
	leal (%ebp, %ebx, INTEGER_SIZE), %esi 
	jmp do_syscall

	
syscall_0arg:
	// 0 arguments, zero %esi
	xorl %esi, %esi
	// make the system call
	jmp do_syscall
syscall_1arg:
	// Put single arg into %esi
	movl (%ebp, %ebx, ARG_PKT_OFFSET), %esi 		

do_syscall:
	// Move base address of int table into ebx
	movl $syscall_int_table, %ebx
	// Move table enttry size into eax
	movl $INT_TABLE_ENTRY_SIZE, %eax
	// sub SYSCALL_INT from interrupt number in %ecx
	sub $SYSCALL_INT, %ecx
	// Find the int table offset in bytes
	// table entry size * interrupt entry number
	mull %ecx
	addl %ebx, %eax
	// Jump to address %ebx + %eax, i.e. table address
	ljmp *(%eax)
	

/**
 * Epilogue wrapper
 */
syscall_wrapper_end:	
	/* restore the callee save registers */
	popl	%esi
	popl	%ebx
	popl	%edi
	movl	%ebp,%esp
	popl	%ebp
	ret
