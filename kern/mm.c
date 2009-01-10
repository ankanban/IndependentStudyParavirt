#define DEBUG_LEVEL KDBG_INFO

#include <stddef.h>
#include <task.h>
#include <sched.h>
#include <kernel_syscall.h>
#include <vmm.h>
#include <sched.h>
#include <usercopy.h>
#include <common_kern.h>
#include <kdebug.h>

int 
sys_new_pages(void * sysarg)
{
  kdinfo("Entered new_pages");
  uint32_t addr = (uint32_t)copy_user_int(sysarg,
					  0);


  if (addr < USER_MEM_START ||
      addr >= VMM_KERNEL_BASE) {
    return -1;
  }

  
  if ((addr & PAGE_MASK) != addr) {
    return -1;
  }
  
  int size = copy_user_int(sysarg,
			  1);
  
  if (size <= 0 || (size % PAGE_SIZE) != 0) {
    return -1;
  }
  unsigned int num_ppf = machine_phys_frames();

  unsigned int kernel_pages = USER_MEM_START >> PAGE_SHIFT;

  unsigned int user_lomem_pages = num_ppf - kernel_pages - 
    VMM_KERNEL_STACK_PAGES;

  if ((size >> PAGE_SHIFT) >= user_lomem_pages) {

    return -1;
  }


  
  uint32_t limit = VMM_KERNEL_BASE - addr;
  
  if (size > limit) {
    return -1;
  }
 
  task_t * current_task = sched_get_current_thread()->task;
  

  vm_area_t * vma = find_vm_area_overlap(&current_task->vm_area_list,
					 (void *)addr,
					 size);

  if (vma != NULL) {
    return -1;
  }
  
  kdinfo("Creating New VMA for requested memory");

  vma = vm_area_alloc(&current_task->vm_area_list,
		      current_task,
		      (void *)addr,  
		      size,
		      PGTAB_ATTRIB_USR_RW,
		      VMA_TYPE_ZEROPAGE,
		      0,
		      NULL);
    
  if (vma == NULL) {
    return -1;
  }

  /* Force a TLB Flush */
  sched_set_user_pgdir();

  return 0;
}



int 
sys_remove_pages(void * addr)
{
  task_t * current_task = sched_get_current_thread()->task;

  vm_area_t * vma = find_vm_area(&current_task->vm_area_list,
				 addr);
  
  if (vma == NULL) {
    return -1;
  }

  if (addr != vma->start_address) {
    return -1;
  }

  vm_area_remove(&current_task->vm_area_list,
		 vma);

  /* Force a TLB Flush */
  sched_set_user_pgdir();

  return 0;
}


