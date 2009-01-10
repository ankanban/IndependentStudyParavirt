#ifndef __THREAD_H__
#define __THREAD_H__

#include <vm_area.h>
#include <stdint.h>
#include <variable_queue.h>
#include <variable_hash.h>

struct task;

#define THREAD_DEFAULT_STACK_SIZE (1024 << PAGE_SHIFT)
#define THREAD_STACK_ALIGN (0xffffffe0UL)

typedef struct regs {
  uint32_t eax;
  uint32_t ebx;
  uint32_t ecx;
  uint32_t edx;
  uint32_t esi;
  uint32_t edi;
} regs_t;

typedef enum {
  THREAD_STATE_SUSPENDED = -4,
  THREAD_STATE_WAITING = -3,
  THREAD_STATE_ZOMBIE = -2,
  THREAD_STATE_DONE = -1,
  THREAD_STATE_RUNNABLE = 0
} thread_state_t;


struct thread;

Q_NEW_HEAD(thread_list_t, thread);

typedef struct thread {
  
  unsigned int id;
  int count;
  thread_state_t state;
  int deadline;
  int status;


  /* Stack state */
  uint32_t user_stack_size;
  uint32_t kernel_stack_size;

  vm_area_t * vma_kernel_stack;
  vm_area_t * vma_user_stack;

  uint32_t kernel_esp;
  uint32_t kernel_esp0;
  uint32_t esp;
  uint32_t eip;
  uint32_t ss;
  uint32_t ebp;
  uint32_t eflags;

  regs_t regs;

  void * entry_point;

  struct task * task;
  struct thread * parent;

  /* List of threads waiting on this thread */
  thread_list_t wait_list;

  /* Hack to get wait to work */
  thread_list_t self_wait_list;

  /* List of active child threads */
  thread_list_t child_list;

  /* List of terminated child threads */
  thread_list_t zombie_list;

  /* List ptr for child thread list */
  Q_NEW_LINK(thread) child_link;
  
  /* List ptr for thread list */
  Q_NEW_LINK(thread) task_link;

  /* List ptr for run queue */	      
  Q_NEW_LINK(thread) runnable_link;

  /* wait list link */
  Q_NEW_LINK(thread) wait_link;

  /* Thread hash map */
  Q_NEW_LINK(thread) threadmap_link;
} thread_t;



/* Set hash table size to twice the
 * Maximum task map size, for better
 * load factor
 */
#define THREAD_MAP_SIZE (VMM_MAX_THREADS << 1)

HASH_NEW(thread_map_t, 
	 thread_list_t, 
	 int, 
	 thread_t,
	 THREAD_MAP_SIZE);


thread_t *
thread_alloc();

void 
thread_get(thread_t * thread);

void
thread_put(thread_t * thread);

void
thread_free(thread_t * thread);

thread_t * 
thread_create(struct task * task,
	      thread_t * parent,
	      unsigned int user_stack_size,
	      unsigned int kernel_stack_size);

thread_t *
thread_copy(struct task * dst_task,
	    thread_t * src_thread);

void
thread_reset(thread_t * thread,
	     char ** argvec);


void
thread_save_state(uint32_t eip, 
		  uint32_t esp);

void
thread_vanish_noyield(thread_t * thread);

void
thread_vanish(thread_t * thread);

int
thread_yield(thread_t * thread);

int
thread_wait(int * status);

#endif
