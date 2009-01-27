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
#define FORCE_DEBUG
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
#include <process.h>
#include <xen_time.h>
#include <xen_events.h>
#include <xen_bitops.h>
#include <xen/features.h>
#include <xen/version.h>
#include <kdebug.h>

/*
 * state for kernel memory allocation.
 */
extern lmm_t malloc_lmm;

/*
 * Info about system gathered by the boot loader
 */
extern struct multiboot_info boot_info;


start_info_t *xen_start_info;

shared_info_t *xen_shared_info;

extern struct xencons_interface * xen_console;

unsigned long hypervisor_virt_start;

void * xen_store;

void xen_init_console(void);


unsigned char xen_features[XENFEAT_NR_SUBMAPS * 32];

void setup_xen_features(void)
{
    xen_feature_info_t fi;
    int i, j;

    for (i = 0; i < XENFEAT_NR_SUBMAPS; i++) 
    {
        fi.submap_idx = i;
        if (HYPERVISOR_xen_version(XENVER_get_features, &fi) < 0)
            break;
        
        for (j=0; j<32; j++)
            xen_features[i*32+j] = !!(fi.submap & 1<<j);
    }
}


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


    kdinfo( "Hello from a brand new kernel!" );

    /*
     * Install the timer handler function.
     */
    handler_install(kernel_tick,
		    sched_bottom_half);

    vmm_kernel_init();

    xen_init_events();

    /* Enable events */
    __sti();

    setup_xen_features();

    xen_init_console();
    
    xen_init_time();

    kdinfo("Console Initialized");    
    
    sched_init();

    return 0;
}
