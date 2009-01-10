#ifndef __EXCEPTION_WRAPPER_H__
#define __EXCEPTION_WRAPPER_H__
#include <kernel_asm.h>

#define EXW_ERRFLAG_OFFSET 1

#define DECL_EXCEPTION_WRAPPER(name) \
  void name##_exception_wrapper(void)

#define EXW_PROLOG				\
  pushl %ebp;					\
  movl %esp, %ebp;				\
  pusha;					\
  call set_kernel_segments;			\
  call set_kernel_ss;				\
  call lock_kernel;				
  /*call vmm_set_kernel_pgdir;*/			



#define EXW_EPILOG				\
  call sched_save_kernel_esp0;			\
  /*call sched_set_user_pgdir;*/		\
  call unlock_kernel;				\
  call set_user_segments;			\
  popa;						\
  popl %ebp;					\
  addl $ARG_DWORD_SIZE,%esp;			\
  iret;


#define DEFINE_EXCEPTION_WRAPPER(name)          \
  .global name##_exception_wrapper;		\
  name##_exception_wrapper:                     \
  EXW_PROLOG;                                   \
  movl $EXW_ERRFLAG_OFFSET, %ebx;		\
  movl (%ebp, %ebx, ARG_DWORD_SIZE), %ebx;	\
  pushl %ebx;					\
  call name##_exception_handler;		\
  popl %ebx;					\
  EXW_EPILOG

#endif /* __EXCEPTION_WRAPPER_H__ */
