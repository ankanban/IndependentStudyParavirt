
#define DEBUG_LEVEL KDBG_INFO

#include <handler_install.h>
#include <kernel_exceptions.h>
#include <stddef.h>
#include <x86/cr.h>
#include <vmm_page.h>
#include <vm_area.h>
#include <vmm.h>
#include <task.h>
#include <thread.h>
#include <sched.h>
#include <console.h>
#include <xen/xen.h>
#include <kdebug.h>

extern shared_info_t * xen_shared_info;

#define NUM_HANDLERS 20
static void (*exception_wrappers[])(void) = {
  div_exception_wrapper, //0
  dbg_exception_wrapper, //1
  generic_exception_wrapper, //2
  generic_exception_wrapper, //3
  ovf_exception_wrapper, //4
  generic_exception_wrapper, //5
  inv_exception_wrapper, //6
  generic_exception_wrapper, //7
  dbl_exception_wrapper, //8
  generic_exception_wrapper, //9
  generic_exception_wrapper, //10
  seg_exception_wrapper, //11
  stk_exception_wrapper, //12
  gpf_exception_wrapper, // 13
  pgf_exception_wrapper, // 14
  NULL, //15
  generic_exception_wrapper, //16
  generic_exception_wrapper, //17
  generic_exception_wrapper, // 18
  generic_exception_wrapper, // 19
};

char emsg_segfault[] =  "Segmentation Fault";

void
generic_exception_handler(uint32_t errflag)
{
  kdtrace("Exception received: 0x%x", errflag);
  thread_t * current_thread = sched_get_current_thread();
  if (current_thread != NULL) {
	task_t * current_task = current_thread->task;
	current_thread->status = -2;
	putbytes(emsg_segfault, sizeof(emsg_segfault));
	putbyte('\n');
	task_vanish(current_task);
  }
}

void
exceptions_init(void)
{

  int i = 0;
  for (i = 0; i < NUM_HANDLERS; i++) {
    set_exception_entry(i, exception_wrappers[i]);
  }
  
  return;
}


void 
div_exception_handler(uint32_t errflag)
{
  kdtrace("div Exception received: 0x%x", errflag);
  thread_t * current_thread = sched_get_current_thread();
  if (current_thread != NULL) {
    task_t * current_task = current_thread->task;
    	current_thread->status = -2;
	putbytes(emsg_segfault, sizeof(emsg_segfault));
	putbyte('\n');
	task_vanish(current_task);
  }

}


void
dbg_exception_handler(uint32_t errflag)
{
  kdtrace("dbg Exception received: 0x%x", errflag);
}

void
ovf_exception_handler(uint32_t errflag)
{
  kdtrace("ovf Exception received: 0x%x", errflag);
  thread_t * current_thread = sched_get_current_thread();
  if (current_thread != NULL) {
	task_t * current_task = current_thread->task;
	current_thread->status = -2;
	putbytes(emsg_segfault, sizeof(emsg_segfault));
	putbyte('\n');
	task_vanish(current_task);
  }
  
}

void
inv_exception_handler(uint32_t errflag)
{
  kdtrace("inv Exception received: 0x%x", errflag);
  thread_t * current_thread = sched_get_current_thread();
  if (current_thread != NULL) {
    task_t * current_task = current_thread->task;
    current_thread->status = -2;
    putbytes(emsg_segfault, sizeof(emsg_segfault));
    putbyte('\n');
    task_vanish(current_task);
  }
}

void
dbl_exception_handler(uint32_t errflag)
{
  kdtrace("dbl Exception received: 0x%x", errflag);
}

void
seg_exception_handler(uint32_t errflag)
{
  kdtrace("seg Exception received: 0x%x", errflag);
  thread_t * current_thread = sched_get_current_thread();
  if (current_thread != NULL) {
	task_t * current_task = current_thread->task;
	current_thread->status = -2;
	putbytes(emsg_segfault, sizeof(emsg_segfault));
	putbyte('\n');
	task_vanish(current_task);
  }
}

void
stk_exception_handler(uint32_t errflag)
{
  kdtrace("stk Exception received: 0x%x", errflag);
  thread_t * current_thread = sched_get_current_thread();
  if (current_thread != NULL) {
    task_t * current_task = current_thread->task;
    current_thread->status = -2;
    putbytes(emsg_segfault, sizeof(emsg_segfault));
    putbyte('\n');
    task_vanish(current_task);
  }

}


void
gpf_exception_handler(uint32_t errflag)
{
  kdtrace("gpf Exception received: 0x%x", errflag);

  thread_t *  current_thread = sched_get_current_thread();
  task_t * task = current_thread->task;
  
  kdinfo("Entered gpf handler: Current thread: %p, tid:0x%x, pid: 0x%x",
	 current_thread,
	 current_thread->id,
	 task->id);
  
  kdtrace("exception details: eip: 0x%x, eflags: 0x%x, esp: 0x%x",
	  current_thread->eip,
	  current_thread->eflags,
	  current_thread->esp);
  
  if (current_thread != NULL) {
    current_thread->status = -2;
    putbytes(emsg_segfault, sizeof(emsg_segfault));
    putbyte('\n');
    task_vanish(task);
  }

}


/* Only works for a single Virtual CPU */
uint32_t
xen_get_cr2(void)
{

  if (xen_shared_info->vcpu_info[0].evtchn_upcall_pending != 0) {
    xen_shared_info->vcpu_info[0].evtchn_upcall_pending = 0;
  }

  return xen_shared_info->vcpu_info[0].arch.cr2;
}

void
pgf_exception_handler(uint32_t errflag)
{
  //uint32_t cr2 = get_cr2();

  uint32_t cr2 = xen_get_cr2();

  uint32_t vpf = cr2 & PAGE_MASK;
  
  kdinfo("pgf Exception received: err: 0x%x, cr2: 0x%x, pg: 0x%x", 
	  errflag,
	  cr2,
	  vpf);
  thread_t *  current_thread = sched_get_current_thread();
  task_t * task = current_thread->task;
  
  kdinfo("Entered pgf handler: Current thread: %p, 0x%x",
	  current_thread,
	  current_thread->id);
  
  /* CR2 contains faulting virtual address */
  // - Read the faulting virtual address
  // - get the current thread/task
  // - Panic if its a super-visor mode fault or other unhandled errors
  // - find the VMA for the virtual address
  //    - Crash if not found
  // - call the nopage() handler for the VMA if the page was absent
  // - call the pagewrte() handler for the VMA if the page was not writableo
  // - Repeat for next VMA until all VMAs for this page 
  // have been handled

  
  if (!(errflag & PGF_ERR_USR)){
    panic("Page fault in Supervisor mode! @ %p, err:0x%x",
	   cr2, errflag);
  }

  if ((errflag & PGF_ERR_RSVD)){
    kdinfo("Reserved bits sete in PTE/PDE @ %p, err:0x%x",
	   cr2, errflag);
  }

  kdinfo("About to search vm area list");


  vm_area_t * vma = find_vm_area(&task->vm_area_list,
				 (void *)cr2);
  
  int handled = 0;

  for (; vma != NULL; vma = Q_GET_NEXT(vma, task_link)) {
    kdinfo("testing vma: %p:%d",
	   vma,
	   vma->type);
    if (((uint32_t)vma->start_address & PAGE_MASK) <= vpf &&
	(((uint32_t)vma->start_address + vma->size - 1) & PAGE_MASK) >= vpf) {
      kdinfo("Found vma: %p, start: %p, size: %p",
	      vma,
	      vma->start_address,
	      vma->size);
      int rc = 0;
      if (!(errflag & PGF_ERR_ACCESS)) {
	/* The page was not present,
	 * try to load it
 	 */
	handled = 1;
	rc = vma->nopage(vma, (void *)cr2);
	if (rc < 0) {
	  handled = 0;
	  break;
	}
	if (vma->type != VMA_TYPE_COW){
	  continue;
	}
      } 
      if (errflag & PGF_ERR_WR) {
	handled = 1;
	/*
	 * the page was not writable, make it so
	 */
	rc = vma->pagewrite(vma, (void *)cr2);
	if (rc < 0) {
	  handled = 0;
	  break;
	}
      }
    } else {
      break;
    }
  }
  if (handled) {
    /*
     * Refresh the page mapping before we
     * exit the page fault handler   
     */
    kdinfo("Handled Page fault, exiting handler");
    vmm_set_user_pgdir(task->page_dir);
  }  else {
    kdinfo("Unhandled page fault");
    thread_t * current_thread = sched_get_current_thread();
    if (current_thread != NULL) {
      task_t * current_task = current_thread->task;
      current_thread->status = -2;
      putbytes(emsg_segfault, sizeof(emsg_segfault));
      putbyte('\n');
      task_vanish(current_task);
    } else { 
      panic("Unhandled page fault");
      
    }
  }
  //sched_yield();
}


