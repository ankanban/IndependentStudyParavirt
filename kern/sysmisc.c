
#define DEBUG_LEVEL KDBG_INFO

#include <stddef.h>
#include <process.h>
#include <task.h>
#include <sched.h>
#include <kernel_syscall.h>
#include <vmm.h>
#include <kdebug.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <malloc.h>
#include <exec2obj.h>
#include <loader.h>
#include <elf_410.h>
#include <usercopy.h>


void 
sys_halt(void)
{
  sched_halt();
}

int 
sys_ls(void * sysarg)
{
  int size = copy_user_int(sysarg, 0);
  int length = 0;
  int i = 0;
  
  kdtrace("Entered sys_ls");
  exec2obj_userapp_TOC_entry const * toc_entry = NULL;
  int offset = 0;
  int num_entries = 0;
  for (i = 0; i < exec2obj_userapp_count; i++) {
    toc_entry = &exec2obj_userapp_TOC[i];
    length = strlen(toc_entry->execname);
    kdtrace("Exec: %s", toc_entry->execname);
    /* Need space for '\0'*/
    if ((offset + length + 1) > size) {
      return -1;
    }
    kdinfo("offset:%d length:$d, size: %d",
	   offset, length, size);
    kdtrace("Cpying to userspace");

    copy_buf_to_user_argpkt_buf((char *)toc_entry->execname,
				offset,
				length,
				sysarg,
				1);
    offset += length;
    

    copy_char_to_user_argpkt_buf('\0',
				 offset,
				 size,
				 sysarg,
				 1);
    offset++;
    num_entries++;
  }

  return num_entries;
}

/* "Special" */
void 
sys_misbehave(void * sysarg)
{
  
}
