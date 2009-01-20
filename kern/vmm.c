
#define DEBUG_LEVEL KDBG_INFO

#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <x86/page.h>
#include <x86/cr.h>
#include <vmm_page.h>
#include <vmm.h>
#include <common_kern.h>
#include <stdint.h>
#include <variable_queue.h>
#include <file.h>
#include <usercopy.h>
#include <kdebug.h>

/*
 * Exit point for all newly created
 * threads, defined in kernel_asm.S
 */
void exit_kernel(void);

void
vmm_kernel_init()
{
  vmm_page_init();
  return;
}


int
vmm_task_load_kernel(task_t * task)
{
  vm_area_src_t src;
  /* The physical address we want to map 
   */
  src.paddr = 0;
  unsigned int num_phys_mem_ppf = machine_phys_frames();
  unsigned int num_kernel_text_ppf = (USER_MEM_START >> PAGE_SHIFT);
  
  /*
   * Map the kernel text segment in the 
   * low memory region.
   */
  vm_area_t * vma_lm_kern = vm_area_alloc(&task->vm_area_list,
					  task,
					  0,
					  (num_kernel_text_ppf << PAGE_SHIFT),
					  PGTAB_ATTRIB_SU_RW,
					  VMA_TYPE_DIRECT,
					  0,
					  &src);
  
  if (vma_lm_kern == NULL) {
    return -1;
  }

  /*
   * Map the virtual address range starting from 
   * VMM_KERNEL_BASE, which is equal in size to the physical
   * memory, to the physical memory map.
   */
  vm_area_t * vma_phys_mem = vm_area_alloc(&task->vm_area_list,
					   task,
					   (void *)(VMM_KERNEL_BASE),
					   (num_phys_mem_ppf << PAGE_SHIFT),
					   PGTAB_ATTRIB_SU_RW,
					   VMA_TYPE_DIRECT,
					   0,
					   &src);
  if (vma_phys_mem == NULL) {
    vm_area_remove(&task->vm_area_list,
		   vma_lm_kern);
    return -1;
  }

  return 0;
  
}


/* 
 * Create the basic virtual memory map for the task
 */
int
vmm_task_create(task_t * task)
{
  pde_t * upbdr = NULL ;

  task->cs = SEGSEL_USER_CS;
  task->ds = SEGSEL_USER_DS;

  upbdr = vmm_create_pgdir();
  if (upbdr == NULL) {
    kdinfo("Unable to allocate tsask page dir");
    return -1;
  }

  task->page_dir = upbdr;
  
  kdinfo("User Pgdir base: %p", upbdr);
  
  Q_INIT_HEAD(&task->vm_area_list);

  vmm_set_user_pgdir(task->page_dir);

  /*
  if (vmm_task_load_kernel(task) < 0) {
    kdinfo("Unable to allocate task direct mapped segments");
    vmm_free_pgdir(task->page_dir);
    return -1;
  }
  */
  return 0;
}


int
vmm_task_copy(task_t * dst_task,
	      task_t * src_task)
{
  
  vm_area_t * curpos = NULL;
  vm_area_t * vma = NULL;
  vm_area_src_t vma_src;
 

  Q_FOREACH(curpos, &src_task->vm_area_list, task_link) {
    vma = NULL;
    vma_src.vma = curpos;
    
    if (curpos->type == VMA_TYPE_DIRECT) {
      continue;
      /* Already done during task creation */      
    } else if (curpos->type == VMA_TYPE_KERNEL_STACK) {
      /* Kernel stacks are allocated during
       * thread creation
       */
      continue;
      
    } else {
      
      /* Default is to just copy the VMA contents from
       *  the other VMA.
       */
      vma = vm_area_alloc(&dst_task->vm_area_list,
			  dst_task,
			  curpos->start_address,
			  curpos->size,
			  curpos->flags,
			  VMA_TYPE_COPY,
			  curpos->vma_flags,
			  &vma_src);

      if (vma == NULL) {
	kdinfo("Unable to copy VMA at %p from task %p:%d for task %p:%d",
	       curpos->start_address,
	       src_task,
	       src_task->id,
	       dst_task,
	       dst_task->id);
	return -1;
      }
    }
  }
  return 0;
}

void
vmm_setup_kernel_stack(thread_t * dst_thread)
{
  // Kernel stack is always direct mapped, so we can use the
  // address directly.
  // Also, kernel stack ptr starts on an integer boundary.
  uint32_t * kstack = (uint32_t *)(dst_thread->kernel_esp0);

  kdtrace("setup_kernel_stack: kstack=%p", kstack);
   
  *kstack = SEGSEL_USER_DS;
  *(--kstack) = dst_thread->esp;
  *(--kstack) = dst_thread->eflags;
  *(--kstack) = SEGSEL_USER_CS;
  *(--kstack) = dst_thread->eip;
  *(--kstack) = dst_thread->ebp;
  uint32_t current_kebp = (uint32_t)(kstack);
  *(--kstack) = (uint32_t)exit_kernel;
  *(--kstack) = current_kebp;

  int i = 0;
  for (; i < 6; i++) {
    /* Save values for eax through esi */
    *(--kstack) = 0;
  }
  
  dst_thread->kernel_esp = (uint32_t)(kstack); 

  kdinfo("kernel ESP set to: %p", 
	  (void *)dst_thread->kernel_esp);
  
  kdinfo("Thread EIP: %p", 
	  dst_thread->eip);
  
  kdinfo("Thread return addr: %p", 
	  exit_kernel);
}

/*
 * Create the kernel stack for the thread
 */
int
vmm_kernel_stack_init(thread_t * thread)
{
  uint32_t stack_size = thread->kernel_stack_size;

  kdtrace("Creating Kernel Stack VMA");

  vm_area_t * kstack_vma = vm_area_alloc(&thread->task->vm_area_list,
					 thread->task,
					 NULL,
					 stack_size,
					 PGTAB_ATTRIB_SU_RW,
					 VMA_TYPE_KERNEL_STACK,
					 VMA_FLAG_STACK,
					 NULL);


  if (kstack_vma == NULL) {
    return -1;
  }

  kdtrace("Created Kernel stack VMA %p", kstack_vma);
  vm_area_get(kstack_vma);

  kdtrace("Incremented Kstack VMA refcnt: %d",
	  kstack_vma->count);


  thread->vma_kernel_stack = kstack_vma;

  kdtrace("Setting up kernel ESP0");

  thread->kernel_esp0 = ((uint32_t)kstack_vma->start_address + kstack_vma->size - 1) & 
    THREAD_STACK_ALIGN;

  thread->kernel_esp = thread->kernel_esp0;
  
  kdinfo("Kernel ESP0 for thread %d is: 0x%x",
	  thread->id,
	  thread->kernel_esp0);

  kdtrace("About to fix up kernel stack");

  vmm_setup_kernel_stack(thread);

  return 0;

}


void
vmm_copy_argvec(thread_t * thread,
		char ** argvec)
{
  int argc = 0;
  int argvec_str_size = 0;
  int argvec_array_size = 0;

  get_string_array_dim(argvec,
		       &argc,
		       &argvec_array_size,
		       &argvec_str_size);
  
  kdinfo("argc: %d", argc); 
  
  kdinfo("argvec_array_size: %d", argvec_array_size);
  
  char ** stack_argvec = (char **)(thread->esp - 
				   argvec_str_size -
				   argvec_array_size);

  kdinfo("stack_argvec: %p", stack_argvec);

  char * stack_argvec_str = (char *)(thread->esp -
				     argvec_str_size);
  kdinfo("stack_argvec_str: %p", stack_argvec_str);

  int i = 0;

  for (; i < argc; i++) {
    stack_argvec[i] = stack_argvec_str;
    strcpy(stack_argvec_str,
	   argvec[i]);
    kdinfo("copied str: %p:\"%s\"", 
	   stack_argvec_str,
	   stack_argvec_str);
    stack_argvec_str += strlen(stack_argvec[i]) + 1;
  }
  
  stack_argvec[i] = NULL;

  thread->esp = (uint32_t)stack_argvec;
  
  thread->esp -= sizeof(char **);

  *((char ***)thread->esp) = stack_argvec;

  thread->esp -= sizeof(argc);

  *((int *)thread->esp) = argc;
  
  thread->esp -= sizeof(uint32_t);

  *((uint32_t *)thread->esp) = 0;

  kdinfo("Thread user ESP set to: 0x%x", 
	 thread->esp);

}


void
vmm_setup_user_stack(thread_t * thread, 
		     char ** argvec)
{
  // Set the thread's esp 
  uint32_t stack_size = thread->user_stack_size;
  
  void * start_address = (void *)(VMM_KERNEL_BASE - 
				  stack_size - 
				  PAGE_SIZE);
  
  thread->esp = ((uint32_t)start_address + stack_size - 1) & THREAD_STACK_ALIGN;
  kdinfo("Set Thread user ESP: 0x%x",
	 thread->esp);
  
  /*
   * Copy the argvec
   */
  if (argvec != NULL) {
    vmm_copy_argvec(thread,
		    argvec);
  }
}


int
vmm_user_stack_init(thread_t * thread)
{
  task_t * task = thread->task;
  uint32_t stack_size = thread->user_stack_size;
  
  /* Keep a one-page buffer between user stack and kernel stack */
  /* XXX for multiple threads, we should do something better,
   *  and try to allocate using this  as the base address 
   */
  void * start_address = (void *)(VMM_KERNEL_BASE - 
				  stack_size - 
				  PAGE_SIZE);

  vm_area_t * vma = vm_area_alloc(&task->vm_area_list,
				  task,
				  start_address,  
				  stack_size,
				  PGTAB_ATTRIB_USR_RW,
				  VMA_TYPE_ZEROPAGE,
				  VMA_FLAG_STACK,
				  NULL);

  if (vma == NULL) {
    kdinfo("Unable to create user stack");
    return -1;
  }
  
  thread->vma_user_stack = vma;
  vm_area_get(vma);

  vmm_setup_user_stack(thread, NULL);

  kdinfo("vmm_user_stack_init exit");
  
  return 0;
}


int
vmm_thread_copy(thread_t * dst_thread,
		thread_t * src_thread)
{
  dst_thread->esp = src_thread->esp;
  dst_thread->ebp = src_thread->ebp;
  dst_thread->eip = src_thread->eip;
  dst_thread->eflags = src_thread->eflags;
  dst_thread->user_stack_size = src_thread->user_stack_size;
  dst_thread->kernel_stack_size = src_thread->kernel_stack_size;

  /* Get the reference to the user stack */
  kdinfo("About to get user stack reference");
  vm_area_t * uvma = find_vm_area(&dst_thread->task->vm_area_list,
				  src_thread->vma_user_stack->start_address);

  assert(uvma != NULL);
  
  dst_thread->vma_user_stack = uvma;
  vm_area_get(uvma);

  // Create the kernel stack for this thread
  // Also set the thread->esp
  kdinfo("About to set up thread kernel stack (no user stack setup)");
  if (vmm_kernel_stack_init(dst_thread) < 0) {
    return -1;
  }
  
  
  kdinfo("thread Kernel ESP: 0x%x", 
	  dst_thread->kernel_esp);
  kdinfo("thread ESP: 0x%x", 
	  dst_thread->esp);
  
  return 0;

}


/* Create a Stack at the very top of the
 * address space. 
 * XXX To support thread_fork,
 * should allocate a stack by 
 * dynamically allocating pages, and mapping it into
 * first available high mem area, with some guard pages
 * for adjacent stacks.
 */
int
vmm_thread_create(thread_t * thread, 
		  unsigned int user_stack_size,
		  unsigned int kernel_stack_size)
{

  thread->esp = 0;
  thread->ebp = 0;
  thread->eip = (uint32_t)thread->task->entry_point;
  thread->eflags = 0x202; // Find out what to set here...
  thread->user_stack_size = user_stack_size;
  thread->kernel_stack_size = kernel_stack_size;

  kdinfo("About to set up thread user stack");

  if (vmm_user_stack_init(thread) < 0) {
    return -1;
  }
  
  kdinfo("About to set up thread kernel stack");
  // Create the kernel stack for this thread
  

  if (vmm_kernel_stack_init(thread) < 0) {
    return -1;
  }

  kdinfo("thread Kernel ESP: 0x%x", 
	  thread->kernel_esp);
  
  kdinfo("thread ESP: 0x%x", 
	  thread->esp);
  return 0;
}


int
vmm_load_text(task_t * task,
	      simple_elf_t * se_hdr,
	      file_t * file)
{ 
  vm_area_src_t src;

  src.file_src.file = file;
  src.file_src.start_offset = se_hdr->e_txtoff;
  
  if (se_hdr->e_txtlen == 0) {
      return -1;
  }
    

  vm_area_t * vma = vm_area_alloc(&task->vm_area_list,
				  task,
				  (void *)se_hdr->e_txtstart,
				  se_hdr->e_txtlen,
				  PGTAB_ATTRIB_USR_RO,
				  VMA_TYPE_FILE,
				  0,
				  &src);
  if (vma == NULL) {
    kdinfo("vmm:Could not load Text");
    return -1;
  }

  return 0;
}


int
vmm_load_rodata(task_t * task,
		simple_elf_t * se_hdr,
		file_t * file)
{
  vm_area_src_t src;

  if (se_hdr->e_rodatlen == 0) {
    kdinfo("No rodata");
    return 0;
  }

  src.file_src.file = file;
  src.file_src.start_offset = se_hdr->e_rodatoff;

  vm_area_t * vma = vm_area_alloc(&task->vm_area_list,
				  task,
				  (void *)se_hdr->e_rodatstart,
				  se_hdr->e_rodatlen,
				  PGTAB_ATTRIB_USR_RO,
				  VMA_TYPE_FILE,
				  0,
				  &src);
  if (vma == NULL) {
    kdinfo("vmm:Could not load rodata");
    return -1;
  }
  
  return 0;
}


int
vmm_load_data(task_t * task,
	      simple_elf_t * se_hdr,
	      file_t * file)
{
  vm_area_src_t src;

  if (se_hdr->e_datlen == 0) {
    kdinfo("no data");
    return 0;
  }

  src.file_src.file = file;
  src.file_src.start_offset = se_hdr->e_datoff;

  vm_area_t * vma =  vm_area_alloc(&task->vm_area_list,
				   task,
				   (void *)se_hdr->e_datstart,
				   se_hdr->e_datlen,
				   PGTAB_ATTRIB_USR_RW,
				   VMA_TYPE_FILE,
				   0,
				   &src);
  if (vma == NULL) {
    kdinfo("vmm:Could not load data");
    return -1;
  }

  return 0;
}

int
vmm_load_bss(task_t * task,
	     simple_elf_t * se_hdr,
	     file_t * file)
{
  vm_area_src_t src;

  if (se_hdr->e_bsslen == 0) {
    kdinfo("no data");
    return 0;
  }


  vm_area_t * vma = vm_area_alloc(&task->vm_area_list,
				  task,
				  (void *)(se_hdr->e_datstart + 
					   se_hdr->e_datlen),
				  se_hdr->e_bsslen,
				  PGTAB_ATTRIB_USR_RW,
				  VMA_TYPE_ZEROPAGE,
				  0,
				  &src);
  if (vma == NULL) {
        kdinfo("vmm:Could not load BSS");
    return -1;
  }
  
  return 0;
}
