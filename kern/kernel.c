/** @file kernel.c
 *  @brief An initial kernel.c
 *
 *  You should initialize things in kernel_main(),
 *  and then run stuff.
 *
 *  @author Harry Q. Bovik (hqbovik)
 *  @author Fred Hacker (fhacker)
 *  @bug No known bugs.
 */
#define DEBUG_LEVEL KDBG_INFO
#include <common_kern.h>

/* libc includes. */
#include <stdio.h>
#include <simics.h>                 /* lprintf() */
#include <malloc.h>
#include <stddef.h>

/* multiboot header file */
#include <multiboot.h>              /* boot_info */

/* memory includes. */
#include <lmm.h>                    /* lmm_remove_free() */

/* x86 specific includes */
#include <x86/seg.h>                /* install_user_segs() */
#include <x86/interrupt_defines.h>  /* interrupt_setup() */
#include <x86/asm.h>                /* enable_interrupts() */

/*Xen includes*/
#include <stdint.h>
#include <xen/xen.h>

#include <interrupt_wrappers.h>
#include <kernel_timer.h>
#include <console.h>
#include <keyboard.h>
#include <timer.h>
#include <handler_install.h>
#include <task.h>
#include <thread.h>
#include <vmm.h>
#include <syscall_int.h>
#include <kernel_asm.h>
#include <sched.h>
#include <kernel_syscall.h>
#include <process.h>
#include <kdebug.h>

/*
 * state for kernel memory allocation.
 */
extern lmm_t malloc_lmm;

/*
 * Info about system gathered by the boot loader
 */
extern struct multiboot_info boot_info;


shared_info_t *HYPERVISOR_shared_info;

unsigned long hypervisor_virt_start;

/** @brief Kernel entrypoint.
 *  
 *  This is the entrypoint for the kernel.
 *
 * @return Does not return
 */
int 
kernel_main(start_info_t *mbinfo, int argc, char **argv, char **envp)
{
    /*
     * Tell the kernel memory allocator which memory it can't use.
     * It already knows not to touch kernel image.
     */

    HYPERVISOR_shared_info = mbinfo;

    /*
     * Install the timer handler function.
     */
    handler_install(kernel_tick,
		    sched_bottom_half);


    enable_interrupts();

    syscall_init();

    /*
     * initialize the PIC so that IRQs and
     * exception handlers don't overlap in the IDT.
     */
    interrupt_setup();

    /*
     * When kernel_main() begins, interrupts are DISABLED.
     * You should delete this comment, and enable them --
     * when you are ready.
     */

    lprintf( "Hello from a brand new kernel!" );

    vmm_kernel_init();
    
    sched_init();

    return 0;
}
