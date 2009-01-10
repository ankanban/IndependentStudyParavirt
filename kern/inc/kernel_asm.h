#ifndef __KERNEL_ASM_H__
#define __KERNEL_ASM_H__

#include <x86/seg.h>


#define DEFINE_SET_SEG_PROTO(reg)		\
  void set_kernel_##reg(void);			\
  void set_user_##reg(void);

#define DEFINE_ALL_SET_SEG_PROTO \
  DEFINE_SET_SEG_PROTO(ds)	 \
  DEFINE_SET_SEG_PROTO(es)	 \
  DEFINE_SET_SEG_PROTO(fs)	 \
  DEFINE_SET_SEG_PROTO(gs)	 \
  DEFINE_SET_SEG_PROTO(ss)        


#define DEFINE_SET_SEG(seg, reg)		\
  .global set_kernel_##reg;			\
set_kernel_##reg:				\
 pushl %ebx;					\
 mov $SEGSEL_KERNEL_##seg, %bx;			\
 mov %bx, %##reg;				\
 popl %ebx;					\
 ret;                                           \
 .global set_user_##reg;			\
set_user_##reg:                                 \
 pushl %ebx;					\
 mov $SEGSEL_USER_##seg, %bx;			\
 mov %bx, %##reg;				\
 popl %ebx;					\
 ret;

#define DEFINE_ALL_SET_SEG			  \
  DEFINE_SET_SEG(DS, ds);			  \
  DEFINE_SET_SEG(DS, es);			  \
  DEFINE_SET_SEG(DS, fs);			  \
  DEFINE_SET_SEG(DS, gs);                         \
  DEFINE_SET_SEG(DS, ss);                         

#define ARG_DWORD_SIZE 4
#define ARG_WORD_SIZE  2
#define ARG_BYTE_SIZE  1

#define EX_EIP_OFFSET     2
#define EX_EFLAGS_OFFSET  3
#define EX_ESP_OFFSET     4
#define EX_PAGEDIR_OFFSET 5

#define SCW_EIP_OFFSET     1
#define SCW_CS_OFFSET      2
#define SCW_EFLAGS_OFFSET  3
#define SCW_ESP_OFFSET     4
#define SCW_SS_OFFSET      5

#define DECL_SCW(scsmall) \
  void sys_##scsmall##_wrapper(void)

#define PUT_SYSCALL(sccaps, scsmall)			\
  do {							\
    set_syscall_entry(sccaps##_INT, sys_##scsmall##_wrapper);	\
  } while (0)

#define KASM_BREAK \
  movl $0x1badd00d, %ebx;			\
  xchg %ebx, %ebx;				


#define SCW_PROLOG				\
  pushl %ebp;					\
  movl %esp, %ebp;				\
						\
  /*call SIM_break;*/				\
  call set_kernel_segments;			\
  call set_kernel_ss;				\
  call lock_kernel;
/*call vmm_set_kernel_pgdir;*/			


#define SCW_EPILOG				 \
  pushl %eax;					 \
  call sched_save_kernel_esp0;                   \
  /*call sched_set_user_pgdir;*/		 \
  call unlock_kernel;				 \
  call set_user_segments;			 \
  popl %eax;					 \
  /*call SIM_break;*/				 \
  popl %ebp;					 \
  iret;

#define DEFINE_SYSCALL_WRAPPER(sc)		\
  .global sys_##sc##_wrapper;			\
  sys_##sc##_wrapper:				\
  SCW_PROLOG;					\
  /* Save the user eip, stack pointer & eflags 	\
   * Need to do this if the syscall		\
   * forks, so the new thread can use the same	\
   * eip+eflags and an 'equivalent' esp		\
   * XXX don't have to do this since we copy    \
   * stack whole.				\
   */						\
						\
  movl $SCW_EFLAGS_OFFSET, %eax;		\
  movl (%ebp, %eax, ARG_DWORD_SIZE), %ecx;	\
  pushl %ecx;					\
						\
  movl (%ebp), %ecx;				\
  pushl %ecx;					\
						\
  movl $SCW_ESP_OFFSET, %eax;		        \
  movl (%ebp, %eax, ARG_DWORD_SIZE), %ecx;	\
  pushl %ecx;					\
						\
  movl $SCW_EIP_OFFSET, %eax;			\
  movl (%ebp, %eax, ARG_DWORD_SIZE), %ecx;	\
  pushl %ecx;					\
						\
  						\
  call sched_save_thread_user_state;		\
  						\
  popl %ecx;					\
  popl %ecx;					\
  popl %ecx;					\
  popl %ecx;					\
						\
  /*call SIM_break;*/				\
						\
  pushl %esi;					\
  call sys_##sc;				\
  popl %esi;					\
						\
  SCW_EPILOG					




#endif /*__KERNEL_ASM_H__ */
