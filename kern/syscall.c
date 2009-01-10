#include <kernel_asm.h>
#include <kernel_syscall.h>
#include <handler_install.h>
#include <syscall_int.h>
#include <kdebug.h>

void
syscall_init()
{

  ktrace("Setting up system call vectors");
  
  PUT_SYSCALL(GETTID, gettid);

  PUT_SYSCALL(FORK, fork);

  PUT_SYSCALL(EXEC, exec);
  
  PUT_SYSCALL(SET_STATUS,set_status);

  PUT_SYSCALL(VANISH,vanish);

  PUT_SYSCALL(WAIT,wait);

  PUT_SYSCALL(TASK_VANISH,task_vanish);

  PUT_SYSCALL(YIELD,yield);

  PUT_SYSCALL(CAS2I_RUNFLAG,cas2i_runflag);

  PUT_SYSCALL(GET_TICKS,get_ticks);

  PUT_SYSCALL(SLEEP,sleep);

  PUT_SYSCALL(NEW_PAGES,new_pages);

  PUT_SYSCALL(REMOVE_PAGES,remove_pages);

  PUT_SYSCALL(GETCHAR,getchar);

  PUT_SYSCALL(READLINE,readline);

  PUT_SYSCALL(PRINT,print);

  PUT_SYSCALL(SET_TERM_COLOR,set_term_color);

  PUT_SYSCALL(SET_CURSOR_POS,set_cursor_pos);

  PUT_SYSCALL(GET_CURSOR_POS,get_cursor_pos);

  PUT_SYSCALL(HALT,halt);

  PUT_SYSCALL(LS,ls);

  PUT_SYSCALL(MISBEHAVE,misbehave);
}
