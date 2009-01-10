/*#define FORCE_DEBUG*/
#define DEBUG_LEVEL KDBG_INFO

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <task.h>
#include <vmm.h>
#include <x86/cr.h>
#include <x86/eflags.h>
#include <x86/seg.h>
#include <simics.h>
#include <elf_410.h>
#include <sched.h>
#include <file.h>
#include <kdebug.h>

int current_task_id = 0;


void
print_elf_header(simple_elf_t * se_hdr)
{
  kdinfo("Name         : %s", se_hdr->e_fname);
  kdinfo("Entry        : %p", se_hdr->e_entry);
  kdinfo("txtoff       : %p", se_hdr->e_txtoff);
  kdinfo("e_txtlen     : %u", se_hdr->e_txtlen);
  kdinfo("e_txtstart   : %p", se_hdr->e_txtstart);    /* start of text segment virtual address */
  kdinfo("e_datoff     : %p", se_hdr->e_datoff);      /* offset of data segment in file */
  kdinfo("e_datlen     : %p", se_hdr->e_datlen);      /* length of data segment in bytes */
  kdinfo("e_datstart   : %p", se_hdr->e_datstart);    /* start of data segment in virtual memory */
  kdinfo("e_rodatoff   : %p", se_hdr->e_rodatoff);    /* offset of rodata segment in file */
  kdinfo("e_rodatlen   : %u", se_hdr->e_rodatlen);    /* length of rodata segment in bytes */
  kdinfo("e_rodatstart : %p", se_hdr->e_rodatstart);  /* start of rodata segment in virtual memory*/
  kdinfo("e_bsslen     : %u", se_hdr->e_bsslen);      /* length of bss  segment in bytes */

}


task_t *
task_alloc()
{
  return (task_t *)malloc(sizeof(task_t));
}

void
task_free(task_t * task)
{
  kdinfo("TASK delete");  

  /* Put files */
  if (task->file != NULL) {
    kdinfo("task_free: Releasing executable file");
    file_put(task->file);
  }
  
  /* Free page tables */ 
  kdinfo("task_free: deleting page dir");
  if (task->page_dir != NULL) {
    vmm_free_pgdir(task->page_dir);
  }

  /* Free parent */
  if (task->parent != NULL) {
    kdinfo("task_free: Releasing parent task");
    task_put(task->parent);
  }

  kdinfo("task_free: Freeing task %p", task);
  free(task);
}

void
task_get(task_t * task)
{
  task->count++;
  kdinfo("REFCNT INC TASK %p:%d", task, task->count);
}

void
task_put(task_t * task)
{
  task->count--;
  kdinfo("REFCNT DEC TASK %p:%d", task, task->count);
  assert(task->count >= 0);
  if (task->count == 0) {
    task_free(task);
  }
}



/*
 * loads a task and sets the entry point
 * for it
 * 
 */
int 
task_load(task_t * task,
	  char * task_path)
{ 
  /* Use getbytes to load the executable's pages */
  simple_elf_t * se_hdr = (simple_elf_t *)malloc(sizeof(simple_elf_t));

  if (se_hdr == NULL) {
    kdinfo("Failed to alloc se_hdr");
    goto task_load_fail;
  }

  int rc = elf_load_helper(se_hdr, task_path);

  if (rc < 0) {
    kdinfo("Invalid file name");
    goto task_load_fail;
  }

  print_elf_header(se_hdr);

  file_t * file = file_alloc();

  file_open(file, task_path);

  file_get(file);
  task->file = file;

  kdinfo("Loading VM areas from file");

  rc = vmm_load_text(task, se_hdr, file);

  if (rc < 0) {
    kdinfo("Failed to load task");
    goto task_load_fail;
  }

  rc = vmm_load_rodata(task, se_hdr, file);

  if (rc < 0) {
    kdinfo("Failed to load task");
    goto task_load_fail;
  }
  
  rc = vmm_load_data(task, se_hdr, file);

  if (rc < 0) {
    kdinfo("Failed to load task");
    goto task_load_fail;
  }

  rc = vmm_load_bss(task, se_hdr, file);

  if (rc < 0) {
    kdinfo("Failed to load task");
    goto task_load_fail;
  }
  
  kdinfo("Completed VM Area Loads");

  // Set the entry point for the first thread
  task->entry_point = (void *)se_hdr->e_entry;
  
  free(se_hdr);
  
  return 0;

 task_load_fail:
  if (se_hdr != NULL) {
    free(se_hdr);
  }
  return -1;

}

int
task_init(task_t * task,
	  task_t * parent)
{
  memset(task, 0, sizeof(task_t));

  if (sched_find_task(current_task_id + 1) != NULL) {
    return -1;
  }

  task->id = ++current_task_id;

  task->count = 0;
  task->parent = NULL;

  if (parent != NULL) {
    /* Add a ref to the parent task */
    kdinfo("Adding ref to parent from tsk: %p:%d",
	   task,
	   task->id);
    task_get(parent);
    task->parent = parent;
    /* Copy the entry point from parent */
    task->entry_point = parent->entry_point;
    
    /* Add a ref to the source file */
    task->file = parent->file;

    kdinfo("Adding task to parent task");
    /* Add to parent task's child list */
    Q_INSERT_TAIL(&parent->child_list,
		  task,
		  child_link);

    task_get(task);
  }
  
  if (task->file != NULL) {
    file_get(task->file);
  }
  
  Q_INIT_HEAD(&task->vm_area_list);
  Q_INIT_HEAD(&task->child_list);
  
  /* Add task to task map 
   * Also increases task refcount.
   */
  kdtrace("Adding task to task map");
  sched_add_task(task);

  return 0;
}


task_t *
task_create(task_t * parent)
{
  kdtrace("Allocating task");
  task_t * task = task_alloc();

  kdtrace("initializing task");
  if (task_init(task, parent) < 0) {
    /* Task init failure is only handled for
     * overlapping task ids
     */
    kdinfo("Found a existing task with allocated task id");
    free(task);
    return NULL;
  }

  kdtrace("creating page directory/tables");
  
  /* Create page directory and tables */
  if (vmm_task_create(task) < 0) {
    kdinfo("Ynable to create task");
    /* Release all the resources held */
    task_reap(task);
    /* Free the task */
    task_free(task);
    return NULL;
  }

  return task;
}


task_t * 
task_copy(task_t * parent)
{
 task_t * task = task_create(parent);

 if (task == NULL) {
   return NULL;
 }

 if (vmm_task_copy(task,
		   parent) < 0) {
   /* Release task resources */
   kdinfo("Unable to copy task");
   task_reap(task);
   /* Delete the task */
   task_free(task);
   return NULL;
 }
 
 kdinfo("TASK PGDIR: %p",
	task->page_dir);

 return task;
}


void
task_delete_vm(task_t * task)
{
  vm_area_remove_all(&task->vm_area_list);
}

int
task_reload(task_t * task,
	    thread_t * thread,
	    char * path,
	    char * argvec[])
{
  vm_area_t * uvma = thread->vma_user_stack;

  assert(uvma != NULL);

  /* Zero out the mapped in portions of the stack */
  vm_area_zero(uvma,
	       uvma->start_address,
	       uvma->size);
  
  /* First copy the argvec to the stack before 
   * we lose the old vm mappings (argvec may come
   * from the heap)
   */
  vmm_setup_user_stack(task->thread,
		       argvec);  
  
  /* Deletes everything but thread stacks,
   * and direct mappings
   */
  vm_area_remove_taskvm(&task->vm_area_list);
   
  if (task->file != NULL) {
    file_put(task->file);
    task->file = NULL;
  }

  kdtrace("Loading new VM for the task");
  
  /* Load the task */
  int rc = task_load(task,
		     path);

  if (rc < 0) {
    kdinfo("Unable to load task from path %s", path);
    task->thread->status = -1;
    task_vanish(task);
    /* task_vanish also switches to another thread/task */
  }

  kdtrace("Resetting entry point for thread");

  thread_reset(task->thread, argvec); 

  return 0;
}

void
task_reap(task_t * task)
{

  /* Remove from task map
   * This also should cause the task to be marked for
   * deletion (actually done when the thread is cleaned up).
   */
  kdtrace("Removing task from task map");

  sched_remove_task(task);
  
  /* Dissociate with parent task */
  if (task->parent != NULL) {
    Q_REMOVE(&task->parent->child_list,
	     task,
	     child_link);
    task_put(task);
    task_put(task->parent);
    task->parent = NULL;
  }

  
  /* Dissociate with  all the active children for this task */
  task_t * child_task = Q_GET_FRONT(&task->child_list);

  /* Release all the child tasks */
  for (; child_task != NULL;) {
    task_t * next_task = child_task->child_link.next;
    
    kdinfo("Releasing parent ref from task : %p:%d",
	   task, task->id);
    Q_REMOVE(&task->child_list,
	     child_task,
	     child_link);
    task_put(child_task);
    child_task = next_task;
  } 


  /*
   * We should not delete the kernel stack until
   * we have switched to the new thread.
   * this means the final thread_put should do this
   * Done from the parent thread, when it terminates,
   * or reaps the zombie thread
   */


  /* Remove VMAs */
  kdtrace("task_free: Deleting VMs for the task %p",
	  task);
  
  /*
   * Delete all the Virtual Memory Areas
   * for the task. Actually, removes them
   * from the task list, and decrements 
   * the refcount. Most VMAs will get cleaned
   * up, except for the kernel stack VMA, which
   * is also owned by the thread. That gets deleted
   * when the thread is deleted (if the thread hasn't been
   * created yet, then the Kernel stack VMA gets deleted too).
   * Direct mapped VMAs such as the kernel region/phys memmap
   * are deleted, but the mappings in the page table are preserved
   * These are only deleted when the parent thread cleans up the
   * thread and task. This is because we need the mappings and 
   * the kernel stack for one last context switch.
   */
  task_delete_vm(task);

}

void
task_vanish_noyield(task_t * task)
{
  /*
   * For multiple threads, we would walk the list,
   * and call thread_vanish_noyield for each thread.
   */
  if (task->thread != NULL &&
      task->thread->state != THREAD_STATE_DONE) {
    /* The thread removes itself from parent's
     * child list, and adjusts its own reference
     * accordingly, so no thread_put required here.
     */
    thread_vanish_noyield(task->thread);
  }
  
}

void
task_vanish(task_t * task)
{
  task_vanish_noyield(task);

  sched_yield();
}
