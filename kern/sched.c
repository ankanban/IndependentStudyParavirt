
#define DEBUG_LEVEL KDBG_INFO

#include <x86/cr.h>
#include <x86/asm.h>
#include <kernel.h>
#include <task.h>
#include <thread.h>
#include <sched.h>
#include <process.h>
#include <assert.h>
#include <lock.h>
#include <vmm.h>
#include <kdebug.h>

mutex_t kernel_lock = MUTEX_LOCKED;

thread_t * current_thread = NULL;
thread_list_t sched_runnable_queue;
thread_list_t sched_sleep_queue;
thread_list_t sched_done_queue;

int kernel_booting = 0;

task_map_t *sched_task_map;
thread_map_t *sched_thread_map;

task_t * sched_idle_task = NULL;
thread_t * sched_idle_thread = NULL;
char * sched_idle_task_path = "idle";

//char * init_task_path = "fork_test1";
//char * init_task_path = "exec_basic";
//har * init_task_path = "fork_wait";
char * init_task_path = "init";

task_t * sched_init_task;
thread_t * sched_init_thread;

void
sched_scan_sleep_queue();


int
trylock_kernel()
{
  kdverbose("Trying kernel lock");
  int rc = mutex_trylock(&kernel_lock);
  if (!rc) {
    kdverbose("Failed to acquire kernel lock");
  } else {
    kdverbose("Acquired kernel lock");
  }
  return rc;
}

int
lock_kernel()
{
  kdverbose("Trying kernel lock");
  int rc = mutex_spinlock(&kernel_lock);
  if (!rc) {
    kdverbose("Failed to acquire kernel lock");
  } else {
    kdverbose("Acquired kernel lock");
  }
  return rc;
}

void
unlock_kernel()
{
  kdverbose("Releasing kernel lock");
  mutex_unlock(&kernel_lock);
  kdverbose("Released Kernel lock");
}

void
sched_lock()
{
  //disable_interrupts();
}

void
sched_unlock()
{
  //enable_interrupts();
}

/*
 * Jenkins Hash
 */

uint32_t 
jenkins_hash(int id)
{
  uint32_t hash = 0;
  unsigned char * key = (unsigned char *)&id;
  int i = 0;
  for (i = 0; i < sizeof(id); i++) {
    hash += key[i];
    hash += (hash << 10);
    hash ^= (hash >> 6);
  }
  hash += (hash << 3);
  hash ^= (hash >> 11);
  hash += (hash << 15);

  return hash;
}

int
sched_hashcmp(int id1, int id2)
{
  return (id1 == id2)?(0):((id1 > id2)?(1):(-1));
}

uint32_t
sched_task_hashfn(int taskid)
{
  kdinfo("Hashfn: taskid:%d", taskid);
  uint32_t hash = jenkins_hash(taskid);;
 
  hash &= (TASK_MAP_SIZE - 1);

  kdinfo("Hashfn: hash:0x%x", hash);
  return hash;
}


int
sched_task_hashkey(task_t * task)
{
  return task->id;
}

uint32_t
sched_thread_hashfn(int thread_id)
{
  kdinfo("Hashfn: threadid:%d", thread_id);
  uint32_t hash = jenkins_hash(thread_id);;
 
  hash &= (THREAD_MAP_SIZE - 1);

  kdinfo("Hashfn: hash:0x%x", hash);
  return hash;
}


int
sched_thread_hashkey(thread_t * thread)
{
  return thread->id;
}



void
sched_start_init()
{
  kdtrace("Creating first task");
  
  sched_init_task = task_create(NULL);
  
  kdtrace("Creating first thread");
  
  sched_init_task->thread = thread_create(sched_init_task,
					  NULL,
					  THREAD_DEFAULT_STACK_SIZE,
					  VMM_KERNEL_THREAD_STACK_SIZE);
  
  sched_init_thread = sched_init_task->thread;
  thread_get(sched_init_thread);
    
    /*
     * Start off the first task
     */
  do_exec(sched_init_task,
	  sched_init_task->thread,
	  init_task_path,
	  NULL);
  

  kdinfo("init_thread: kesp: 0x%x, kesp0:0x%x, esp:0x%x, eip:0x%x",
	 sched_init_task->thread->kernel_esp,
	 sched_init_task->thread->kernel_esp0,
	 sched_init_task->thread->esp,
	 sched_init_task->thread->eip); 
  
  
  kdtrace("Adding runnable thread");
  
  sched_add_runnable_thread(sched_init_task->thread);
  
  kdtrace("Fixing kernel stack");
  
  vmm_setup_kernel_stack(sched_init_task->thread);
  
  kdtrace("About to enter user mode");
  
  sched_yield();

}

void sched_start_idle()
{
    
  sched_idle_task = task_create(NULL);
  
  kdtrace("Creating IDLE thread");
  
  sched_idle_thread = thread_create(sched_idle_task,
				    NULL,
				    THREAD_DEFAULT_STACK_SIZE,
				    VMM_KERNEL_THREAD_STACK_SIZE);
  
  sched_idle_task->thread = sched_idle_thread;
  
  thread_get(sched_idle_thread);
  
  task_reload(sched_idle_task,
	      sched_idle_thread,
	      sched_idle_task_path,
	      NULL);

  vmm_setup_kernel_stack(sched_idle_thread);
  
  kdinfo("Created IDLE process");

}

void
sched_init()
{
  current_thread = NULL;

  kernel_booting = 1;
  
  Q_INIT_HEAD(&sched_runnable_queue);
  
  Q_INIT_HEAD(&sched_done_queue);
  
  Q_INIT_HEAD(&sched_sleep_queue);
  
  kdinfo("Creating hash tables of size tsk:%d, thr:%d",
	 sizeof(task_map_t), sizeof(thread_map_t));

  sched_task_map = vmm_ppfs_alloc((sizeof(task_map_t) + PAGE_SIZE) >> 
				  PAGE_SHIFT);

  sched_thread_map = vmm_ppfs_alloc((sizeof(thread_map_t) + PAGE_SIZE) >> 
				    PAGE_SHIFT);
  
  sched_task_map->hashfn = sched_task_hashfn;

  sched_task_map->hash[0].first = NULL;

  sched_task_map->hash[0].last = NULL;

  HASH_INIT(sched_task_map,
	    TASK_MAP_SIZE,
	    sched_task_hashfn,
	    sched_hashcmp,
	    sched_task_hashkey);

  HASH_INIT(sched_thread_map,
	    THREAD_MAP_SIZE,
	    sched_thread_hashfn,
	    sched_hashcmp,
	    sched_thread_hashkey);

  kdinfo("Creating IDLE process");

  sched_start_idle();

  sched_start_init();

}

void
sched_add_task(task_t * task)
{
  task_get(task);  
  sched_lock();
  kdinfo("Adding task %p:%d to map %p",
	  task, task->id, sched_task_map);
  HASH_INSERT(sched_task_map, (task->id), task, taskmap_link);
  sched_unlock();
}

task_t *
sched_find_task(int taskid)
{
  task_t * task = NULL;
  sched_lock();
  HASH_LOOKUP((sched_task_map), (taskid), (&task), taskmap_link);
  sched_unlock();

  return task;
}

void
sched_remove_task(task_t * task)
{
  sched_lock();
  HASH_REMOVE(sched_task_map, 
	      task->id, 
	      task, 
	      taskmap_link);
  sched_unlock();
  task_put(task);
}

void
sched_add_thread(thread_t * thread)
{
  thread_get(thread);  
  sched_lock();
  kdinfo("Adding thread %p:%d to map %p",
	  thread, thread->id, sched_thread_map);
  HASH_INSERT(sched_thread_map, (thread->id), thread, threadmap_link);
  sched_unlock();
}

thread_t *
sched_find_thread(int thread_id)
{
  thread_t * thread = NULL;
  sched_lock();
  HASH_LOOKUP((sched_thread_map), (thread_id), (&thread), threadmap_link);
  sched_unlock();

  return thread;
}

void
sched_remove_thread(thread_t * thread)
{
  sched_lock();
  HASH_REMOVE(sched_thread_map, 
	      thread->id, 
	      thread, 
	      threadmap_link);
  sched_unlock();
  thread_put(thread);
}


void
sched_remove_runnable_thread(thread_t * thread)
{
  sched_lock();
  Q_REMOVE(&sched_runnable_queue, 
	   thread, 
	   runnable_link);
  sched_unlock();
  thread_put(thread);
}

void
sched_add_runnable_thread(thread_t * thread)
{
  thread_get(thread);
  sched_lock();
  thread->state = THREAD_STATE_RUNNABLE;
  Q_INSERT_TAIL(&sched_runnable_queue, thread, runnable_link);
  sched_unlock();
}

void
sched_remove_waiting_thread(thread_t * thread)
{
  sched_lock();
  Q_REMOVE(&sched_sleep_queue, 
	   thread, 
	   runnable_link);
  sched_unlock();
  thread_put(thread);
}

void
sched_add_waiting_thread(thread_t * thread)
{
  thread_get(thread);
  sched_lock();
  thread->state = THREAD_STATE_RUNNABLE;
  Q_INSERT_TAIL(&sched_sleep_queue, thread, runnable_link);
  sched_unlock();
}



void
sched_remove_done_thread(thread_t * thread)
{

  sched_lock();
  Q_REMOVE(&sched_done_queue, 
	   thread, 
	   runnable_link);
  sched_unlock();
  thread_put(thread);
}

void
sched_add_done_thread(thread_t * thread)
{
  thread_get(thread);
  thread->state = THREAD_STATE_DONE;
  Q_INSERT_TAIL(&sched_done_queue, thread, runnable_link);

}

void
sched_halt(void)
{
  /* Go through the run queue, and call task_vanish
   * for all of the thread/tasks in the queue
   */
  while (1);
  /*
  thread_t * curpos = NULL;
  Q_FOREACH (curpos, &sched_runnable_queue, runnable_link) {
    // task_vanish only kills the task 
    kdinfo("Queueing task %d for termination",
	   curpos->task->id);
    task_vanish_noyield(current_thread->task);
  }
  sched_yield();
*/
}

void
sched_thread_yield(thread_t * new_thread,
		   thread_t * old_thread)
{
  sched_set_current_thread(new_thread);

  kdverbose("Set current thread to %p:%d",
	 new_thread, new_thread->id);

  // sched_unlock();

  /* Switch only if the scheduler found a different runnable 
   * thread to schedule, otherwise, just follow the current
   * return path.
   */
  if (new_thread != old_thread) {
    kdtrace("About to context switch to tid %d, pid %d",
	    new_thread->id,
	    new_thread->task->id);
    
    do_context_switch(old_thread,
		      new_thread->kernel_esp,
		      new_thread->task->page_dir);
  }
  
}

void
sched_yield(void)
{
  //  sched_lock();

  thread_t * old_thread = sched_get_current_thread();
  kdinfo("OLD THREAD: %p:%d", 
	 old_thread,
	 old_thread->id);
  thread_t * new_thread = old_thread;

  /*** Scheduler : Step 1 ***/

  /* First reprioritize the current thread (old_thread) 
   * If no old_thread existed, then we are booting the
   * first task, so set a flag, and avoid saving the
   * throwaway boot-time
   * kernel context during the first task switch.
   */

  if (old_thread == NULL) {
    kernel_booting = 1;
    kdtrace("Kernel booting, first task switch");
  } else {
    /* move to runq if current thread is not sleeping or dead
     * or its not an idle thread.
     * (May have been killed recently by sys_vanish..)
     */
    /* Move the interrupted thread to the back of the queue */
    if (old_thread != sched_idle_thread) {
      kdverbose("Removing Current thread from runqueue");

      if (old_thread->state == THREAD_STATE_RUNNABLE) {
	Q_REMOVE(&sched_runnable_queue, old_thread, runnable_link);
	kdverbose("Moving Current thread to runqueue tail");
	Q_INSERT_TAIL(&sched_runnable_queue, 
		    old_thread,
		      runnable_link);
      } else if (old_thread->state == THREAD_STATE_DONE) {
	Q_REMOVE(&sched_runnable_queue, old_thread, runnable_link);
	kdinfo("Found a done thread");
	kdinfo("This thread will be cleaned up after thread switch");
	Q_INSERT_TAIL(&sched_done_queue,
		      old_thread,
		      runnable_link);
      
      } else {
	kdinfo("Found a waiting thread, not reinserting into runqueue");
      }
    }
  }
  
  kdverbose("Scheduler Step 2");
  /*** Scheduler : Step 2 ***/

  /* Now try to find a new schedulable thread */
  do {
    
    new_thread = Q_GET_FRONT(&sched_runnable_queue);

    if (new_thread == NULL) {
      new_thread = sched_idle_thread;
      //panic("No schedulable threads. Halting!");
      kdverbose("No schedulable threads, scheduling idle_thread");
      break;
    } else if (new_thread->state != THREAD_STATE_RUNNABLE) {
      /* Found a dead thread in the run queue, move it
       * to the 'done' queue, so deal with later
       */
      Q_REMOVE(&sched_runnable_queue, 
	       new_thread, 
	       runnable_link);
      Q_INSERT_TAIL(&sched_done_queue,
		    new_thread,
		    runnable_link);
      new_thread = NULL;
    }
  } while (new_thread == NULL);


  kdverbose("Scheduler step 3");
  /*** Scheduler : Step 3 ***/

  /* We found a schedulable thread, that is not dead,
   * Set it as the current thread, and switch to it
   */

  sched_thread_yield(new_thread, old_thread);
  
  kdverbose("Scheduler Step 4");
  /*** Scheduler : Step 4 ***/

  /* Now cleanup all the dead threads */
  
  thread_t * next_thread = NULL;
  for(old_thread = Q_GET_FRONT(&sched_done_queue);
      old_thread != NULL; old_thread = next_thread) {
    next_thread = old_thread->runnable_link.next;
    kdtrace("Deleting completed thread %p:%d",
	    old_thread, old_thread->id);
    sched_remove_done_thread(old_thread);
  }

  /*
   * Wake up all the threads that need to be woken up.
   */
  sched_scan_sleep_queue();

}

thread_t *
sched_get_current_thread(void)
{
  return current_thread;
}

void
sched_set_current_thread(thread_t * thread)
{
  current_thread = thread;
}

void
sched_save_thread_user_state(uint32_t eip,
			     uint32_t esp,
			     uint32_t ebp,
			     uint32_t eflags)
{
  kdinfo("Saving user thread state: eip:0x%x, esp:0x%x, ebp:0x%x, efl:0x%x",
	 eip,
	 esp,
	 ebp,
	 eflags); 
  
  current_thread->eip = eip;
  current_thread->esp = esp;
  current_thread->ebp = ebp;
  current_thread->eflags = eflags;
}

void
sched_save_thread_kernel_state(thread_t * old_thread,
			       uint32_t kernel_esp)
			       
{
  /* Save kernel esp only after first thread is away */
  if (!kernel_booting && old_thread != NULL) {
    kdtrace("Saving Kernel ESP: th: %p:%d, tsk: %p:%d, esp:0x%0x",
	    old_thread,
	    old_thread->id,
	    old_thread->task,
	    old_thread->task->id,
	    kernel_esp);
    
    old_thread->kernel_esp = kernel_esp;
  }
  kernel_booting = 0;
}

void
sched_save_kernel_esp0(void)
{
  //set_esp0(current_thread->kernel_esp0);

  int ret = HYPERVISOR_stack_switch(SEGSEL_KERNEL_DS,
				    current_thread->kernel_esp0);

  assert(ret == 0);
}

void
sched_set_user_pgdir(void)
{
  vmm_set_user_pgdir(current_thread->task->page_dir);
}

void
sched_wait_on(thread_list_t * thread_list,
	      int ticks,
	      thread_t * thread)
{
  /*
   * Assumption: Thread is in RUNNABLE state
   */

  if (thread->state != THREAD_STATE_RUNNABLE) {
    kdtrace("thread %p:%d, state: %d, not runnable, "
	    "only runnable threads can wait",
	    thread, thread->id, thread->state);
	    
    return;
  }
  
  
  thread->deadline = kernel_getticks() + ticks;
  
  thread->state = THREAD_STATE_WAITING;

  kdtrace("Thread %p:%d, deadline:%d, state:%d, Going to sleep",
	  thread, thread->id, thread->deadline, thread->state);

  /* Since we're only switching queues
   * from runnable to a wait queue,
   * refcount on the thread remains the
   * same for this operation
   */
  Q_REMOVE(&sched_runnable_queue, 
	   thread, 
	   runnable_link);
  
  Q_INSERT_TAIL(thread_list,
		thread,
		wait_link);

  // Switch to a runnable thread
  sched_yield();
  
  kdtrace("Thread %p:%d returned from sleep",
	  thread, thread->id);

  // Return from sleep...
  // Thread has been placed on runnable queue
  // with the appropriate refcount
}

void
sched_sleep(thread_t * thread,
	    int ticks)
{
  kdtrace("Thread %p:%d Going to wait for %d ticks",
	  thread, thread->id, ticks);
  sched_wait_on(&sched_sleep_queue,
		ticks,
		thread);
  
}


void
sched_wakeup_threads(thread_t * waitee,
		     thread_list_t * thread_list)
{
  kdverbose("Entered wakeup_threads for thread %p, list: %p",
	    waitee, thread_list);


  int ticks = kernel_getticks();
  thread_t * curpos = Q_GET_FRONT(thread_list);
  thread_t * nextpos = NULL;

  for (; curpos != NULL; curpos = nextpos) {
    nextpos = curpos->wait_link.next;    
    /* Refcount on the newly runnable thread
     * doesn't change
     */
    kdverbose("thread: %p, id: %d, deadline: %d, currenttm: %d",
	   curpos,
	   curpos->id,
	   curpos->deadline,
	   ticks);

    if (curpos->deadline <= ticks) {
      kdtrace("Wait over for thread %p:%d", 
	     curpos, 
	     curpos->id);
      
      Q_REMOVE(thread_list, curpos, wait_link);
      
      /* If the thread is waiting, make it runnable, and
       * add to runqueue. else add to done queue.
       */
      if (curpos->state == THREAD_STATE_WAITING) {
	kdtrace("Adding thread %p:%d  to run queue",
	       curpos, curpos->id);
	curpos->state = THREAD_STATE_RUNNABLE;
	/* Try to get the sleeping thread scheduled as
	 * soon as possible
	 */
	Q_INSERT_FRONT(&sched_runnable_queue, 
		       curpos, 
		       runnable_link);
      } else {
	kdtrace("Thread %p:%d is a zombie/done",
	       curpos, curpos->state);
	Q_INSERT_TAIL(&sched_done_queue, curpos, runnable_link);
      }
      
      /*
       * Decrement refcount on the thread we were waiting 
       * on.
       */
      //if (waitee != NULL) {
      //thread_put(waitee);
      //}
    }


  }

  kdverbose("Done wakeup_threads");
}

void
sched_scan_sleep_queue()
{
  kdverbose("Waking up sleeping threads");
  sched_wakeup_threads(NULL,
		       &sched_sleep_queue);
}


void
sched_bottom_half()
{
  kdverbose("Entering timer bottom half");
  
  if (!trylock_kernel()) {
    kdverbose("Already in kernel mode, Aborting timer bottom half");
    /*
     * Avoid the bottom half if
     * we are already in the kernel.
     */
    return;
  }

  if (current_thread == NULL) {
    kdtrace("Kernel still booting");
    return;
  }
  
  
  /* Enter Kernel */
  set_kernel_segments();  
  set_kernel_ss();
  
  sched_yield();

  /* Leave Kernel */
  sched_save_kernel_esp0();

  unlock_kernel();

  set_user_segments();
}
