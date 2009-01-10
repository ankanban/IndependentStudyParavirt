#ifndef __TASK_H__
#define __TASK_H__

#include <vm_area.h>
#include <thread.h>
#include <stdint.h>
#include <vmm_page.h>
#include <variable_queue.h>
#include <variable_hash.h>


struct task;

Q_NEW_HEAD(task_list_t, task);

/* The Process Control Block (PCB) */
typedef struct task {
  /* Task definition */
  uint32_t id;
  int count;



  void * page_dir;
  void * entry_point;
  
  /* File from which the task 
   * was loaded 
   */
  file_t * file;
  /* List of allocated VM regions 
   * XXX Currently hard coded
   */
  vm_area_list_t vm_area_list;
 
  /* Segment registers */
  unsigned int cs;
  unsigned int ds;

  struct task * parent;

  /* List of threads belonging to this task
   * XXX currently only one thread
   * The list is unused
   */
  //thread_list_t thread_list;
  thread_t * thread;
  
  task_list_t child_list;

  Q_NEW_LINK(task) taskmap_link;
  
  Q_NEW_LINK(task) child_link;
} task_t;



/* Set hash table size to twice the
 * Maximum task map size, for better
 * load factor
 */
#define TASK_MAP_SIZE (VMM_MAX_TASKS << 1)

HASH_NEW(task_map_t, 
	 task_list_t, 
	 int, 
	 task_t,
	 TASK_MAP_SIZE);

task_t *
task_alloc();

void
task_free(task_t * task);

void
task_get(task_t * task);

void
task_put(task_t * task);

task_t *
task_copy(task_t * parent);

task_t *
task_create(task_t * parent);

int
task_load(task_t * task,
	  char * task_path);

int
task_wait(int * status);

int
task_wait_on(int taskid);

void
task_delete_vm(task_t * task);

int
task_reload(task_t * task,
	    thread_t * thread,
	    char * path,
	    char * argvec[]);


void
task_vanish_noyield(task_t * task);

void
task_vanish(task_t * task);

void
task_reap(task_t * task);

#endif /* __TASK_H__ */
