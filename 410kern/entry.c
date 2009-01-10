/** @file boot/entry.c
 *  @brief Entry point for the C part of the kernel
 *  @author matthewj S2008
 */

#include <common_kern.h>
#include <x86/page.h>
#include <boot/multiboot.h>
#include <boot/util.h>
#include <simics.h>
#include <lmm/lmm.h>
#include <malloc/malloc_internal.h>
#include <assert.h>

#include <kvmphys.h>

/* For FLUX compatibility; see lib/kvmphys.h */
vm_offset_t phys_mem_va = 0;

/* 410 interface compatibility */
static int n_phys_frames = 0;

/** @brief C entry point */
void mb_entry(mbinfo_t *info, void *istack) {
    int argc;
    char **argv;
    char **envp;

    /* Want (kilobytes*1024)/PAGE_SIZE, but definitely avoid overflow */
    n_phys_frames = (info->mem_upper+1024)/(PAGE_SIZE/1024);
    assert(n_phys_frames > (USER_MEM_START/PAGE_SIZE));

    mb_util_lmm(info, &malloc_lmm);  // init()s malloc_lmm internally
    // lmm_dump(&malloc_lmm);

    mb_util_cmdline(info, &argc, &argv, &envp);

    // Having done that, let's tell Simics we've booted.
    SIM_notify_bootup(argv[0]);

    kernel_main(info, argc, argv, envp);
}

int
machine_phys_frames(void)
{
    return (n_phys_frames);
}
