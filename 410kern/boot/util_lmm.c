/* 
 * Copyright (c) 1996 The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 */

#include <boot/multiboot.h>
#include <string/string.h>
#include <malloc/malloc_internal.h>
#include <kvmphys.h>
#include <lmm/lmm_types.h>

#include <common_kern.h>
#include <stdint.h>
#include <xen/xen.h>

/* The 410 world does not care about memory in this much detail.  If we
 * ever start caring this much, these definitions should move back
 * to their own header file so that other users of LMM can see them
 * too.
 * 
 * At the momemnt, though, given only malloc, these are somewhat silly.
 */

/*
 * <1MB memory is most precious, then <16MB memory, then high memory.
 * Assign priorities to each region accordingly
 * so that high memory will be used first when possible,
 * then 16MB memory, then 1MB memory.
 */
#define LMM_PRI_1MB     -2
#define LMM_PRI_16MB    -1
#define LMM_PRI_HIGH    0

/*
 * For memory <1MB, both LMMF_1MB and LMMF_16MB will be set.
 * For memory from 1MB to 16MB, only LMMF_16MB will be set.
 * For all memory higher than that, neither will be set.
 */
#define LMMF_1MB    0x01
#define LMMF_16MB   0x02

static struct lmm_region reg1mb, reg16mb, reghigh;

#define skip(hole_min, hole_max)					\
	if ((max > (hole_min)) && (min < (hole_max)))			\
	{								\
		if (min < (hole_min)) max = (hole_min);			\
		else { min = (hole_max); goto retry; }			\
	}

/* Extract from xen/xen.h :
 * Start-of-day memory layout:
 *  1. The domain is started within contiguous virtual-memory region.
 *  2. The contiguous region ends on an aligned 4MB boundary.
 *  3. This the order of bootstrap elements in the initial virtual region:
 *      a. relocated kernel image
 *      b. initial ram disk              [mod_start, mod_len]
 *      c. list of allocated page frames [mfn_list, nr_pages]
 *      d. start_info_t structure        [register ESI (x86)]
 *      e. bootstrap page tables         [pt_base, CR3 (x86)]
 *      f. bootstrap stack               [register ESP (x86)]
 *  4. Bootstrap elements are packed together, but each is 4kB-aligned.
 */
void mb_util_lmm (start_info_t *mbi, void * bstack, lmm_t *lmm)
{
  //vm_offset_t min;
  //extern char _start[], end[];

  /*
	vm_offset_t ramdisk_start_pa = mbi->mod_start;
	vm_offset_t ramdisk_end_pa = ramdisk_start_pa ? 
	  (ramdisk_start_pa + mbi->mod_len):
	  0;
      
	vm_offset_t mfnlist_start_pa = mbi->mfn_list;
	vm_offset_t mfnlist_end_pa = mfnlist_start_pa ? 
	  (mfnlist_start_pa + mbi->nr_pages * sizeof(uint32_t)):
	  0;

	vm_offset_t startinfo_start_pa = (vm_offset_t)mbi;
	vm_offset_t startinfo_end_pa = startinfo_start_pa + sizeof(*mbi);


	vm_offset_t bspgtab_start_pa = mbi->pt_base;
  */

	vm_offset_t bspgtab_end_pa = (vm_offset_t)bstack;

	/* Initialize the base memory allocator
	   according to the PC's physical memory regions.  */
	lmm_init(lmm);

	/* Do the x86 init dance to build our initial regions */
	lmm_add_region(&malloc_lmm, &reg1mb,
		       (void*)phystokv(0x00000000), 0x00100000,
		       LMMF_1MB | LMMF_16MB, LMM_PRI_1MB);

	lmm_add_region(&malloc_lmm, &reg16mb,
		       (void*)phystokv(0x00100000), 0x00f00000,
		       LMMF_16MB, LMM_PRI_16MB);

	lmm_add_region(&malloc_lmm, &reghigh,
		       (void*)phystokv(0x01000000), 0xfeffffff,
		       0, LMM_PRI_HIGH);

	
	/* Add region from end of Xen-allocated data structures,
	 * to start of user memory
	 */	
	lmm_add_free(&malloc_lmm, 
		     (void *)bspgtab_end_pa, 
		     USER_MEM_START - bspgtab_end_pa);
	
	/* Everything above 16M */
	lmm_remove_free( &malloc_lmm, (void*)USER_MEM_START, 
		     -8 - USER_MEM_START );
    
	/* Everything below 1M  */
	lmm_remove_free( &malloc_lmm, (void*)0, 0x100000 );


    return;

}

