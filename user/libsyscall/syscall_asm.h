#ifndef __SYSCALL_ASM_H__
#define __SYSCALL_ASM_H__

#include <syscall_int.h>

#define ARG_DWORD_SIZE 4

#define ARG_PKT_OFFSET 2

/*
 * DEFINE_SYSCALL_WRAPPER: Defines a system call wrapper routine.
 * The wrapper handles calling conventions/register saves, loads
 * the argument packet appropriately, depending on number of
 * arguments passed to the system call, calls the interrupt, and
 * cleans up before returning.
 * Return value depends on the function signature of the system call.
 */
#define DEFINE_SYSCALL_WRAPPER(syscall, arg_num, intr_num)		\
  .global syscall;							\
syscall:								\
 /* Save the callee save registers */					\
 pushl	%ebp;								\
 movl	%esp,%ebp;							\
 pushl	%ebx;								\
 pushl	%esi;								\
 pushl	%edi;								\
 									\
 /* Get the number of arguments */					\
 movl $arg_num, %edx;							\
 									\
 /* The argument, put it into ebx now,					\
  * so we can handle for each case					\
  *(0, 1, > 1 arguments)						\
  */									\
 									\
 movl $ARG_PKT_OFFSET, %ebx;						\
 									\
 /* Check if zero, single or multiple arguments */			\
 cmpl $0x1, %edx;							\
 jz syscall_1arg_##syscall;						\
 									\
 /* Check if zero arguments */						\
 cmpl $0x0, %edx;							\
 jz syscall_0arg_##syscall;						\
 									\
 /* Multiple arguments, get arg pkt offset into esi*/			\
 									\
 /* move pkt pointer into %esi*/					\
 leal (%ebp, %ebx, ARG_DWORD_SIZE), %esi;				\
 jmp do_syscall_##syscall;						\
  									\
 									\
syscall_0arg_##syscall:							\
 /* 0 arguments, zero %esi*/						\
 xorl %esi, %esi;							\
 /* make the system call*/						\
 jmp do_syscall_##syscall;						\
 									\
syscall_1arg_##syscall:							\
 /* Put single arg into %esi*/						\
 movl (%ebp, %ebx, ARG_DWORD_SIZE), %esi;				\
 									\
 /* Call the interrupt handler for this syscall */			\
do_syscall_##syscall:							\
 int $intr_num;								\
 									\
 /**									\
  * Epilogue wrapper							\
  */									\
 									\
syscall_wrapper_end_##syscall:						\
 /* restore the callee save registers */				\
 popl	%esi;								\
 popl	%edi;								\
 popl	%ebx;								\
 popl	%ebp;								\
 ret;									

#endif /* __SYSCALL_ASM_H__ */
