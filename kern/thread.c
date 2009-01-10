
#define DEBUG_LEVEL KDBG_INFO

#include <stdint.h>
#include <stdlib.h>
#include <task.h>
#include <vmm.h>
#include <x86/cr.h>
#include <x86/eflags.h>
#include <x86/seg.h>
#include <simics.h>
#include <elf_410.h>
#include <sched.h>
#include <file.h>
#include <string.h>
#include <assert.h>
#include <kdebug.h>

int current_thread_id = 0;

thread_t *
thread_alloc()
{
  return (thread_t *)malloc(sizeof(thread_t));
}

void
thread_free(thread_t * thread)
{
  /* Last VMA to be cleaned up is always
   * The kernel stack 
   * thread_free should result from the 
   * scheduler, right after this thread is killed
   * and we switch to a new thread
   */
  kdinfo("Releasing kstack vma for thread %p:%d",
	 thread, thread->id);

  if (thread->vma_kernel_stack != NULL) {
    vm_area_put(thread->vma_kernel_stack);
  }

  /* Normally deleted from thread_vanish,
   * but put in here in case thread creation
   * is aborted
   */
  if (thread->vma_user_stack != NULL) {
    /* Remove refernce to user stack */
    vm_area_put(thread->vma_user_stack);
    thread->vma_user_stack = NULL;
  }

  kdinfo("Releasing parent ref for thread %p:%d",
	 thread, thread->id);
  
  if (thread->parent != NULL) {
    thread_put(thread->parent);
  }
  
  kdinfo("Releasing task ref for thread %p:%d",
	 thread, thread->id);

  if (thread->task != NULL) {
    task_put(thread->task);
  }

  kdinfo("Freeing thread %p:%d",
	 thread, thread->id);

  free(thread);
}

void 
thread_get(thread_t * thread)
{
  thread->count++;
  kdinfo("REFCNT INC THREAD %p:%d", thread, thread->count);
}

void
thread_put(thread_t * thread)
{
  thread->count--;
  kdinfo("REFCNT DEC THREAD %p:%d", thread, thread->count);
  assert(thread->count >= 0);
  if (thread->count == 0) {
    thread_free(thread);
  }
}

void
thread_reset(thread_t * thread,
	     char ** argvec)
{
  thread->eip = (uint32_t)thread->task->entry_point;
  thread->ss = SEGSEL_USER_DS;
  thread->eflags = 0;
    /* set eflags */
  uint32_t eflags = get_eflags();
  eflags = EFL_CF | EFL_IOPL_RING0 | EFL_IF;
  eflags &= ~EFL_AC;
  thread->eflags = eflags;  
}

int 
thread_init(thread_t * thread,
	    task_t * task,
	    thread_t * parent)
{
  memset(thread, 0, sizeof(thread_t));

  if (sched_find_thread(current_thread_id + 1) != NULL) {
    return -1;
  }

  thread->id = ++current_thread_id;

  thread->count = 0;

  thread->parent = parent;
  /* Add parent's ownership to the thread 
   * Done here because some threads (init/idle)
   * are orphans...
   */
  thread_get(thread);
  if (parent != NULL) {
    /* Add parent ref */
    kdinfo("Adding parent ref for thread %p:%d",
	   thread, thread->id);      
    thread_get(parent); 
    /* Add thread to parent child list */
    kdinfo("Adding parent ownership ref on thread %p:%d",
	   thread, thread->id);
    Q_INSERT_TAIL(&parent->child_list, thread, child_link);
  }

  thread->task = task;

  kdinfo("Adding task ref for thread %p:%d",
	 thread, thread->id);
  
  task_get(task);
  
  thread->kernel_esp = 0;
  thread->kernel_esp0 = 0;
  thread->esp = 0;

  Q_INIT_HEAD(&thread->wait_list);  
  Q_INIT_HEAD(&thread->child_list);
  Q_INIT_HEAD(&thread->zombie_list);

  return 0;
}


thread_t *  
thread_copy(task_t * dst_task,
	    thread_t * src_thread)
{
  thread_t * dst_thread = thread_alloc();
  int rc = 0;

  /* Only thread_init failure handled is duplicate thread ids 
   */
  if (thread_init(dst_thread, 
		  dst_task, 
		  src_thread) < 0) {
    kdinfo("Found an existing thread with allocated id");
    free(dst_thread);
    return NULL;
  }

  rc = vmm_thread_copy(dst_thread,
		       src_thread);

  /* Failed to create thread's resources, delete it */
  if (rc < 0) {
    if (dst_thread->parent != NULL) {
      Q_REMOVE(&dst_thread->parent->child_list, dst_thread, child_link);
    }
    thread_put(dst_thread);
    return NULL;
  }

  kdinfo("Copied a new thread, adding to hashmap");
  sched_add_thread(dst_thread);

  return dst_thread;
}


thread_t * 
thread_create(task_t * task,
	      thread_t * parent,
	      unsigned int user_stack_size,
	      unsigned int kernel_stack_size)
{
  thread_t * thread = thread_alloc();
  int rc = 0;

  /* Initialize thread state */
  if (thread_init(thread, task, parent) < 0) {
    kdinfo("Found an existing thread with allocated id");
    free(thread);
    return NULL;
  }  

  kdtrace("Calling vmm_thread_create");

  /* Initialize thread VM */
  rc = vmm_thread_create(thread, 
			 user_stack_size,
			 kernel_stack_size);
  
  /* Failed to create thread's resources, delete it */
  if (rc < 0) {
    if (thread->parent != NULL) {
      Q_REMOVE(&thread->parent->child_list, thread, child_link);
    }
    thread_put(thread);
    return NULL;
  }

  kdtrace("Created a new thread, adding to hashmap");

  /* Add to hash map */
  sched_add_thread(thread);

  return thread;
}


void
thread_vanish_noyield(thread_t * thread)
{
  /* Remove from hashmap*/
  kdinfo("Removing thread from hashmap");
  sched_remove_thread(thread);

  /* Remove from runqueue */
  kdinfo("Removing thread from runqueue");
  if (thread->state == THREAD_STATE_RUNNABLE) {
    sched_remove_runnable_thread(thread);
  } else if (thread->state == THREAD_STATE_WAITING) {
    sched_remove_waiting_thread(thread);
  }
  
  if (thread->vma_user_stack != NULL) {
    /* Remove refernce to user stack */
    vm_area_put(thread->vma_user_stack);
    thread->vma_user_stack = NULL;
  }
  
  /* Wake up parent if its waiting on children */
  if (thread->parent != NULL) {
    /* Remove thread from child list, and add to zombie list */
    Q_REMOVE(&thread->parent->child_list, 
	     thread, 
	     child_link);
    
    thread->state = THREAD_STATE_ZOMBIE;
    
    /* Add thread to parent zombie list */
    Q_INSERT_TAIL(&thread->parent->zombie_list, 
		  thread, 
		  child_link);
    
    /* Wake up parent if its waiting on
     * this thread
     */
    sched_wakeup_threads(thread->parent,
			 &thread->parent->self_wait_list);
  }
  
  /* Reap all the zombies for this task */
  thread_t * zombie = Q_GET_FRONT(&thread->zombie_list);

  /* Release all the child threads */
  for (; zombie != NULL;) {
    thread_t * next_zombie = zombie->child_link.next;
    kdinfo("Releasing parent ref from thread : %p:%d",
	   thread, thread->id);
    zombie->state = THREAD_STATE_DONE;
    Q_REMOVE(&thread->zombie_list,
	     zombie,
	     child_link);
    /* The thread should be deleted here */
    kdinfo("Releasing zombie ref for thread %p:%d",
	   thread, thread->id);
    thread_put(zombie);
    zombie = next_zombie;
  } 

  /* XXX Not handled: Clean up running child threads
   * So the current assumption is that child processes terminate
   * before the parent.
   * Running child threads should be packed off to the init_thread
   */

  /* Make threads on wait list runnable */
  sched_wakeup_threads(thread,
		       &thread->wait_list);
  

  /* Since there is only one thread,
   * also reap the task's resources
   * XXX With multiple threads, we would check the
   * thread list for the task to see if the list were empty,
   * and reap the task only if the list was empty
   * after the thread was removed from the list.
   */
  kdinfo("Releasing task ownership of thread %p:%d",
	 thread, thread->id);
  thread_put(thread);
  thread->task->thread = NULL;
  task_reap(thread->task);
}

void
thread_vanish(thread_t * thread)
{
  thread_vanish_noyield(thread);
  
  sched_yield();
}

int
thread_yield(thread_t * thread)
{
  thread_t * current_thread = sched_get_current_thread();
  /*
   * Only yield if the target thread is
   * in runnable state.
   */
  if (thread->state != THREAD_STATE_RUNNABLE) {
    return -1;
  }
  
  sched_thread_yield(thread,
		     current_thread);
  return 0;
}


int
thread_wait(int * status)
{
  *status = 0;
  int pid = -1;
  
  thread_t * self = sched_get_current_thread();
  
  if (self == NULL) {
    return -1;
  }

  if (Q_GET_FRONT(&self->child_list) == NULL &&
      Q_GET_FRONT(&self->zombie_list) == NULL) {
    kdinfo("This thread had no children");
    return -1;
  }

  thread_t * zombie = NULL;

  kdinfo("Adding thread ref %p:%d for wait", self, self->id);

  thread_get(self);

  zombie = Q_GET_FRONT(&self->zombie_list);
    
  if (zombie == NULL) {
    kdinfo("Thread %p:%d waiting for child thread",
	   self, self->id);
    sched_wait_on(&self->self_wait_list,
		  0,
		  self);
    zombie = Q_GET_FRONT(&self->zombie_list);
  }
  
  if (zombie != NULL) {

      Q_REMOVE(&self->zombie_list, zombie, child_link);
      
      *status = zombie->status;

      pid = zombie->id;
      
      kdinfo("Releasing zombie ref on wakeup for thread %p:%d",
	     self, self->id);
      thread_put(zombie);

  } 

  kdinfo("Releasing self ref for thread %p:%d",
	 self, self->id);
  thread_put(self);

  return pid;
}
