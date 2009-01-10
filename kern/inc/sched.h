#ifndef __SCHED_H__
#define __SCHED_H__

#include <task.h>
#include <thread.h>
void 
sched_init(void);

void
sched_add_runnable_thread(thread_t * thread);

void
sched_remove_runnable_thread(thread_t * thread);

void
sched_add_waiting_thread(thread_t * thread);


void
sched_remove_waiting_thread(thread_t * thread);

void
sched_add_done_thread(thread_t * thread);

void
sched_remove_done_thread(thread_t * thread);

void
sched_yield(void);

thread_t *
sched_get_current_thread(void);

void
sched_set_current_thread(thread_t * thread);

void
sched_save_thread_kernel_state(thread_t * old_thread,
			       uint32_t kernel_esp);


void
sched_save_thread_user_state(uint32_t eip,
			     uint32_t esp,
			     uint32_t ebp, 
			     uint32_t eflags);

void
sched_set_user_pgdir(void);

void
sched_add_task(task_t * task);

task_t *
sched_find_task(int taskid);

void
sched_remove_task(task_t * task);

void
sched_add_thread(thread_t * thread);

thread_t *
sched_find_thread(int thread_id);

void
sched_remove_thread(thread_t * thread);

void
sched_thread_yield(thread_t * new_thread,
		   thread_t * old_thread);

void
sched_wait_on(thread_list_t * thread_list,
	      int ticks,
	      thread_t * thread);

void
sched_wakeup_threads(thread_t * waitee,
		     thread_list_t * thread_list);

void
sched_sleep(thread_t * thread,
	    int ticks);

void
do_context_switch(thread_t * old_thread,
		  uint32_t new_esp,
		  void * page_dir);

void
sched_bottom_half();

void
sched_halt(void);

int 
lock_kernel(void);

int 
trylock_kernel(void);

void
unlock_kernel(void);

void
exit_exec(uint32_t eip,
	  uint32_t eflags,
	  uint32_t esp,
	  void * pagedir);

#endif /* __SCHED_H__ */
