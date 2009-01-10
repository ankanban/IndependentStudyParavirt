#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <kernel_asm.h>
#include <vmm_page.h>
#include <task.h>
#include <sched.h>

/* Ugly hack to switch pages directories
 * and data/stack segments in the kernel
 */
#define ENTER_KERNEL	 \
  do {			 \
    set_kernel_ds();	 \
    set_kernel_ss();	 \
    vmm_set_kernel_pgdir();	 \
} while (0)

#define LEAVE_KERNEL   \
  do {		       \
    set_user_ds();     \
    set_user_ss();     \
    vmm_set_user_pgdir(sched_get_current_thread()->task->page_dir);	\
} while (0)


DECL_SCW(gettid);
DECL_SCW(exec);
DECL_SCW(fork);
DECL_SCW(set_status);
DECL_SCW(vanish);
DECL_SCW(wait);
DECL_SCW(task_vanish);
DECL_SCW(yield);
DECL_SCW(cas2i_runflag);
DECL_SCW(get_ticks);
DECL_SCW(sleep);
DECL_SCW(new_pages);
DECL_SCW(remove_pages);
DECL_SCW(getchar);
DECL_SCW(readline);
DECL_SCW(print);
DECL_SCW(set_term_color);
DECL_SCW(set_cursor_pos);
DECL_SCW(get_cursor_pos);
DECL_SCW(halt);
DECL_SCW(ls);
DECL_SCW(misbehave);

DEFINE_ALL_SET_SEG_PROTO;

void *
get_return_address(void);

void
syscall_init(void);

#endif /* __SYSCALL_H__ */


