#ifndef __VMM_H__
#define __VMM_H__

/* x86 specific includes */
#include <x86/seg.h>                /* install_user_segs() */
#include <x86/interrupt_defines.h>  /* interrupt_setup() */
#include <x86/asm.h>                /* enable_interrupts() */
#include <x86/page.h>
#include <vmm_page.h>
#include <vm_area.h>
#include <task.h>
#include <elf_410.h>

#define THREAD_STACK_MARGIN 63
#define THREAD_STACK_INT_SHIFT 2

void
vmm_kernel_init();

int
vmm_task_create(task_t * task);

int
vmm_thread_create(thread_t * thread,
		  unsigned int user_stack_size,
		  unsigned int kernel_stack_size);

int
vmm_task_copy(task_t * dst_task,
	      task_t * src_task);

int
vmm_thread_copy(thread_t * dst_thread, 
		thread_t * src_thread);

int
vmm_task_load_kernel(task_t * task);

int
vmm_load_text(task_t * task,
	      simple_elf_t * se_hdr,
	      file_t * file);

int
vmm_load_rodata(task_t * task,
		simple_elf_t * se_hdr,
		file_t * file);

int
vmm_load_data(task_t * task,
	      simple_elf_t * se_hdr,
	      file_t * file);

int
vmm_load_bss(task_t * task,
	     simple_elf_t * se_hdr,
	     file_t * file);

void
vmm_setup_kernel_stack(thread_t * dst_thread);

void
vmm_setup_user_stack(thread_t * thread,
		     char ** argvec);


#endif /* __VMM_H__ */
