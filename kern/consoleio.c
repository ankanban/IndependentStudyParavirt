
#define DEBUG_LEVEL KDBG_INFO

#include <stddef.h>
#include <process.h>
#include <task.h>
#include <sched.h>
#include <kernel_syscall.h>
#include <vmm.h>
#include <kdebug.h>
#include <keyboard.h>
#include <console.h>
#include <usercopy.h>

char 
sys_getchar(void)
{
  return readchar();
}

int 
sys_readline(void * sysarg)
{
  kdinfo("Entered readline");
  int size = copy_user_int(sysarg,
			   0);
  
  int i = 0;
  char ch = 0;
  while (i < (size - 1)) {
    
    while ((ch = readchar()) == -1) {
      sched_yield();
    }

    /* Backspace handling */
    if (ch == '\b') {
      if (i > 0) {
	putbyte(ch);
	i--;
      } 
      continue;
    }

    putbyte(ch);

    kdinfo("Read char: 0x%x, '%c'", ch, ch);
    
    copy_char_to_user_argpkt_buf(ch,
				 i,
				 size,
				 sysarg,
				 1);
    if (ch == '\n' ||
	ch == '\r') {
      kdinfo("Found new line");
      copy_char_to_user_argpkt_buf('\0',
				   i + 1,
				   size,
				   sysarg,
				   1);
      i++;
      break;
    }
    i++;
  }

  kdinfo("returning a line");
  return i;
}

int 
sys_print(void * sysarg)
{
  kdinfo("Entered sys_print");
  int size = copy_user_int(sysarg,
			   0);

  if (size >= PAGE_SIZE) {
    return -1;
  }
  
  char * buf = (char *)copy_user_int(sysarg,
				     1);

  putbytes(buf, size);
  
  return 0;
}

int 
sys_set_term_color(int color)
{
  kdinfo("Entered set_term_color");
  set_term_color(color);
  return 0;
}

int 
sys_set_cursor_pos(void * sysarg)
{
  kdinfo("Entered set_cursor_pos");
  int row = copy_user_int(sysarg,
			  0);
  int col = copy_user_int(sysarg,
			  1);
  kdinfo("Entered set_cursor_pos (%d,%d)",
	 row, col);
  
  return set_cursor(row, col);
}

int 
sys_get_cursor_pos(void * sysarg)
{
  kdinfo("Entered get_cursor_pos");
  /* yet to implement copy to user */
  int row;
  int col;

  get_cursor(&row, &col);

  copy_to_user_argpkt_int(row,
			  sysarg,
			  0);

  copy_to_user_argpkt_int(col,
			  sysarg,
			  1);
  
  return 0;
}
