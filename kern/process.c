
#define DEBUG_LEVEL KDBG_INFO

#include <stddef.h>
#include <process.h>
#include <task.h>
#include <sched.h>
#include <thread.h>
#include <kernel.h>
#include <kernel_syscall.h>
#include <vmm.h>
#include <usercopy.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <kdebug.h>


/* Life Cycle */

int 
sys_fork(void)
{
  int parent_id = 0;
  int child_id = 0;
  thread_t * current_thread = sched_get_current_thread();
  task_t * current_task = current_thread->task;
  task_t * new_task = NULL;
  thread_t * new_thread = NULL;
  
  parent_id = current_task->id;
  
  kdtrace("fork: Entered fork for task: %d, thread: %d",
	  parent_id, current_thread->id);
  
  new_task = task_copy(current_task);


  if (new_task == NULL) {
    return -1;
  }

  kdtrace("fork: Copied new task tsk:%p, id:%d",
	  new_task, new_task->id);


  // Copy user stack
  new_thread = thread_copy(new_task,
			   current_thread);

  
  if (new_thread == NULL) {
    task_vanish_noyield(new_task);
    return -1;
  }
  
  assert((new_thread->vma_user_stack != NULL));
  /* Add new thread to the task */
  new_task->thread = new_thread;
  thread_get(new_thread);

  kdtrace("fork: Copied new thread: thr:%p, id:%d",
	  new_thread, new_thread->id);
   
  
  child_id = new_thread->id;

  sched_add_runnable_thread(new_thread);

  kdtrace("fork: Made new thread runnable");
  
  sched_yield();

  current_thread = sched_get_current_thread();

  kdtrace("fork: current task: tsk:%d, thrd:%d",
	  current_thread->task->id,
	  current_thread->id);

  if (parent_id == current_thread->task->id) {
    kdinfo("fork: return from parent: retval: %d",
	   child_id);
    return child_id;
  }
  
  kdinfo("fork: return from child");
  return 0;
}


int
do_exec(task_t *task, 
	thread_t * thread,
	char * path,
	char ** argvec)
{
  kdtrace("Entered do_exec");
  
  int rc = task_reload(task, thread, path, argvec);

  if (rc < 0) {
    kdinfo("Error loading process");
    return rc;
  }

  return 0;
}

int
sys_exec(void * sysarg)
{
  kdtrace("Entered sys_exec");

  /* All arguments should fit in a 4K page */
  ;
  char * execpath_src = NULL;
  char * execpath = NULL;
  char ** execargv_src = NULL;
  char ** execargv = NULL;

  int argc = 0;
  int argvec_array_size = 0;
  int argvec_str_size = 0;
 
  thread_t * current_thread = sched_get_current_thread();
  task_t * current_task = current_thread->task;

  
  execpath_src = (char *)copy_user_int(sysarg,
				   0);
  kdinfo("Read execpath_src: %s", execpath_src);
  
  /* Copy the execpath */
  execpath = (char *)malloc(strlen(execpath_src) + 1);
  
  if (execpath == NULL) {
    kdinfo("Failed to malloc path buffer");
    goto sys_exec_fail;
  }

  strcpy(execpath, execpath_src);
  
  execargv_src = (char **)copy_user_int(sysarg, 1);
  
  kdtrace("Read execargv_src: %p", execargv_src);

  
  get_string_array_dim(execargv_src,
		       &argc,
		       &argvec_array_size,
		       &argvec_str_size);

  
  execargv = (char **)malloc(argvec_array_size +
			     argvec_str_size);

  if (execargv == NULL) {
    kdinfo("Failed to malloc argv buffer");
    goto sys_exec_fail;
  }


  copy_string_array((char *)execargv,
		    execargv_src,
		    argc,
		    argvec_array_size);

  
  kdtrace("About to do_exec(%p,%p,%p)",
	  current_task,
	  execpath,
	  execargv);

  int rc = do_exec(current_task,
		   current_thread,
		   execpath,
		   execargv);


  if (rc < 0) {
    kdtrace("Error loading new process");
    goto sys_exec_fail;
  }

  free(execpath);
  free(execargv);

  kdtrace("Switching to recycled thread");
  
  exit_exec(current_thread->eip,
	    current_thread->eflags,
	    current_thread->esp,
	    current_thread->task->page_dir);

 sys_exec_fail:
  if (execpath != NULL) {
    free(execpath);
  }
  if (execargv != NULL) {
    free(execargv);
  }
  current_thread->status = -1;
  thread_vanish(current_thread);
  /* Does not return, thread is deleted */
  return -1;
}

int
sys_set_status(int status)
{
  thread_t * current_thread = sched_get_current_thread();
  current_thread->status = status;
  kdinfo("Called set_status");
  return status;
}


void
sys_vanish(void)
{
  kdinfo("Entering sys_vanish");
   
  thread_t * thread = sched_get_current_thread();
 
  kdinfo("VANISH: thread id: %d", thread->id);
 
  thread_vanish(thread);

  /* Does not return */
}

int
sys_wait(int * usr_status_ptr)
{
  int status = 0;

  /* Wait for a child to terminate
   */
  int tid = thread_wait(&status);

  /*
   * Copy result to user.
   */
  copy_to_user_dword((uint32_t)status,
		     (uint32_t)usr_status_ptr);
  
  return tid;
}

void
sys_task_vanish(int tid)
{
  
  kdinfo("Entering sys_task_vanish");
  kdinfo("leaving sys_task_vanish");
  thread_t * thread = sched_find_thread(tid);

  if (thread == NULL) {
    return;
  }

  task_t * task = thread->task;

  if (task == NULL) {
    return;
  }

  task_vanish(task);

  return;
}

/* Thread Management */

int
sys_gettid(void)
{
  thread_t * current_thread = NULL;
    
  current_thread = sched_get_current_thread();

  return current_thread->id;
}

int
sys_yield(int tid)
{
  kdinfo("Called sys_yield");

  if (tid == -1) {
    sched_yield();
    return 0;
  }

  thread_t * thread = sched_find_thread(tid);

  if (thread == NULL) {
    return -1;
  }

  return thread_yield(thread);
}

int 
sys_cas2i_runflag(void * sysarg)
{
  int tid = copy_user_int(sysarg, 0);
  int * oldp = (int *)copy_user_int(sysarg, 1);
  int ev1 = copy_user_int(sysarg, 2);
  int nv1 = copy_user_int(sysarg, 3);
  int ev2 = copy_user_int(sysarg, 4);
  int nv2 = copy_user_int(sysarg, 5);

  thread_t * thread = sched_find_thread(tid);

  if (thread == NULL) {
    kdinfo("c2r: Attempt to control non-existent thread %d",
	   thread->id);
    return -1;
  }
  
  kdinfo("c2r: thread: %p",
	 thread);

  thread_state_t thread_state = thread->state;

  if (thread_state == THREAD_STATE_ZOMBIE ||
      thread_state == THREAD_STATE_DONE) {
    kdinfo("c2r: Attempt to control a completed thread %d",
	   thread->id);
    return -1;
  }

  thread_state_t new_state = THREAD_STATE_RUNNABLE;
  /* Thread is either runnable or suspended */
  if ((ev1 == 0 &&
       thread_state == THREAD_STATE_RUNNABLE) || 
      (ev1 < 0 &&
       thread_state == THREAD_STATE_SUSPENDED)) {
    if (nv1 < 0) {
      new_state = THREAD_STATE_SUSPENDED;
    }    
  } else if ((ev2 == 0 &&
	      thread_state == THREAD_STATE_RUNNABLE) ||
	     (ev2 < 0 &&
	      thread_state == THREAD_STATE_SUSPENDED)) {
    if (nv2 < 0) {
      new_state = THREAD_STATE_SUSPENDED;
    }    
  } 
  
  /* new_state now contains what the new state of the thread should
   * be
   */

  /* A thread is allowed to only suspend itself
   * and only if it is runnable (sanity check)
   */
  if (new_state == THREAD_STATE_SUSPENDED &&
      (thread != sched_get_current_thread() ||
       thread_state != THREAD_STATE_RUNNABLE)) {
    kdinfo("c2r: Invalid cas2i_runflag request on thread",
	   thread, thread->id);
    return -1;
  }
  
  kdinfo("c2r: Saving old value of runflag");
  
  /* Set old value of run flag appropriately */
  if (thread_state == THREAD_STATE_RUNNABLE) {
    copy_to_user_dword(0,
		       (uint32_t)oldp);
  } else {
     copy_to_user_dword((uint32_t)-1,
			(uint32_t)oldp);
  }
  
  /*
   * Do nothing if the thread is already in
   * the requested state.
   */
  if (new_state == thread_state) {
    kdinfo("c2r: Thread already in requested state");
    return 0;
  }
    
  if (new_state == THREAD_STATE_SUSPENDED) {
    kdinfo("c2r: Suspending thread %p:%d",
	   thread, thread->id);
    sched_remove_runnable_thread(thread);
    thread->state = THREAD_STATE_SUSPENDED;
    sched_yield();
    /* Does not return until made runnable */
  } else if (new_state == THREAD_STATE_RUNNABLE) {
    kdinfo("c2r: Recheduling thread %p:%d",
	   thread, thread->id);
    sched_add_runnable_thread(thread);
  }

  return 0;
}

int 
sys_get_ticks(void)
{
  return kernel_getticks();
}

int 
sys_sleep(int ticks)
{
  thread_t * current_thread = sched_get_current_thread();

  sched_sleep(current_thread,
	      ticks);
  
  return 0;
}
