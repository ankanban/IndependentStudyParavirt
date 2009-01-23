/** @file vmm_page.h 
 *
 *  @brief Page table/directory manipulation, allocation
 *
 *  @author Anshuman P.Kanetkar (apk)
 * 
 */
/*#define FORCE_DEBUG*/
#define DEBUG_LEVEL KDBG_INFO

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <x86/page.h>
#include <x86/cr.h>
#include <assert.h>
#include <malloc.h>

#include <vmm_page.h>
#include <common_kern.h>
#include <kdebug.h>
#include <kernel.h>
#include <vmm_zone.h>
#include <thread.h>
#include <task.h>
#include <sched.h>

#include <xen/xen.h>
#include <hypervisor.h>

pde_t * kernel_page_dir;

int init_page_count = 0;

vmm_zone_t himem_zone;
vmm_zone_t lomem_zone;


vmm_zone_t * himem_zone_ptr;
vmm_zone_t * lomem_zone_ptr;

unsigned int num_phys_mem_ppf;
unsigned int num_kernel_text_ppf;
unsigned int num_kernel_text_pdes;
unsigned int num_phys_mem_pdes;

extern start_info_t * xen_start_info;
extern shared_info_t * xen_shared_info;

void ** xen_mfn_list;

#define XEN_UPDATE_QUEUE_SIZE 2048

mmu_update_t xen_update_queue[XEN_UPDATE_QUEUE_SIZE];

int xen_update_pos = 0;

void
vmm_flush_mmu_update_queue()
{
  int count = xen_update_pos;
  int succ_count = 0;

  int ret = HYPERVISOR_mmu_update(xen_update_queue, 
				  count, 
				  &succ_count, 
				  DOMID_SELF);
  
  if (ret < 0) {
    assert(ret == 0);
  }
  
  if (succ_count != count) {
    assert(succ_count == count);
  }
  
  xen_update_pos = 0;

  count = 1;
  succ_count = -1;

  mmuext_op_t op;

  op.cmd = MMUEXT_TLB_FLUSH_LOCAL;

  ret = HYPERVISOR_mmuext_op(&op, 
			     count, 
			     &succ_count, 
			     DOMID_SELF);

  assert(ret == 0);

  assert(succ_count == count);
  

  
}

void
vmm_queue_mmu_update(uint64_t ptr,
		     uint32_t value)
{
  xen_update_queue[xen_update_pos].ptr = ptr;
  xen_update_queue[xen_update_pos].val = value;

  xen_update_pos++;

  if (xen_update_pos == XEN_UPDATE_QUEUE_SIZE) {
    vmm_flush_mmu_update_queue();
  }
  
}


void *
mach_to_phys(void * mach)
{
  uint32_t index = (uint32_t)mach >> PAGE_SHIFT;

  uint32_t offset = (uint32_t)mach & (PAGE_SIZE - 1);
  
  uint32_t pfn = machine_to_phys_mapping[index];
  
  return (void *)((pfn << PAGE_SHIFT) | offset);
}

void * 
phys_to_mach(void * phys)
{
  uint32_t index = (uint32_t)phys >> PAGE_SHIFT;
  uint32_t offset = (uint32_t)phys & (PAGE_SIZE - 1);
  
  uint32_t mfn = (uint32_t)xen_mfn_list[index];

  return (void *)((mfn << PAGE_SHIFT) | offset);
}

int 
vmm_floor_log2(unsigned int n) 
{
  int pos = 0;
  if (n >= 1<<16) { n >>= 16; pos += 16; }
  if (n >= 1<< 8) { n >>=  8; pos +=  8; }
  if (n >= 1<< 4) { n >>=  4; pos +=  4; }
  if (n >= 1<< 2) { n >>=  2; pos +=  2; }
  if (n >= 1<< 1) {           pos +=  1; }
  return ((n == 0) ? (0) : pos);
}

void *
vmm_ppf_highmem_alloc(void)
{
  void * ppfhm = vmm_buddy_get_free_pages(himem_zone_ptr,
					  1);
  return ppfhm;
}

void
vmm_ppf_highmem_free(void * pgaddr)
{
  vmm_buddy_free_pages(himem_zone_ptr,
		       pgaddr, 
		       1);
}

/*
 * num_pages -- should always be a power of 2
 */
void *
vmm_ppfs_highmem_alloc(int num_pages)
{
  kdtrace("himem_alloc entered");
  int order =(vmm_floor_log2(num_pages));
  kdtrace("ppfs_himem: order=%d, num_pages=%d", 
	  order,
	  num_pages);
  void * ppfhm_addr = vmm_buddy_get_free_pages(himem_zone_ptr,
					       order);
  
  return ppfhm_addr;
}

void 
vmm_ppfs_highmem_free(void *pgaddr, int num_pages)
{
  int order = vmm_floor_log2(num_pages);
  vmm_buddy_free_pages(himem_zone_ptr,
		       pgaddr,
		       order);
}

/**
 * 
 * Just return the first available page
 * Used only before the page allocator has
 * been initialized, and paging has been enabled.
 */
void *
vmm_init_ppf_alloc(void)
{
  //void * ppf = (void *)(USER_MEM_START + (init_page_count << PAGE_SHIFT)); 

  /* XXX Page allocation should not reset memory */
  /** If out of pages, and a user request, yield to other process
   */
  /** If a kernel request, something bad has happened */
  //init_page_count++;

  void * ppf = memalign(PAGE_SIZE, PAGE_SIZE);
  return ppf;
}

void *
vmm_init_ppfs_alloc(int num_pages)
{
  int i = 0;
  void * ppf_addr = NULL;
  for (; i < num_pages; i++) {
    if (i == 0) {
      ppf_addr = vmm_init_ppf_alloc();
    } else {
      vmm_ppf_alloc();
    }
  }
  return ppf_addr;
}


void *
vmm_ppf_alloc(void)
{
  void * ppf = vmm_buddy_get_free_pages(lomem_zone_ptr, 1); 

  /* XXX Page allocation should not reset memory */
  /** If out of pages, and a user request, yield to other process
   */
  /** If a kernel request, something bad has happened */
  return ppf;
}

void *
vmm_ppfs_alloc(int num_pages)
{
   int order = vmm_floor_log2(num_pages) + 1;
   kdinfo("ppfs_alloc: Order: %d, num: %d", order, num_pages);
   void * ppf_addr = vmm_buddy_get_free_pages(lomem_zone_ptr,
					      order);
   
   return ppf_addr;
}


void
vmm_ppf_free(void * page_addr)
{
  vmm_buddy_free_pages(lomem_zone_ptr,
		       page_addr,
		       1);
}

void 
vmm_ppfs_free(void *pgaddr, int num_pages)
{
  int order = vmm_floor_log2(num_pages);
  vmm_buddy_free_pages(lomem_zone_ptr,
		       pgaddr,
		       order);
}

void *
vmm_pt_alloc(void)
{
  return vmm_ppf_alloc();
}

/*
 * Creates a page directory, and returns
 * Kernel virtual address of the page.
 *
 */
pde_t *
vmm_create_pgdir()
{
  
  pde_t * page_dir = (pde_t *)vmm_ppf_alloc();
  
  if (page_dir == NULL) {
    return NULL;
  }
  
  kdinfo("Creating pg tables, page_dir: %p",
	 page_dir);

  //memset(page_dir, 0, PAGE_SIZE);
  memcpy(page_dir, kernel_page_dir, PAGE_SIZE);
  

  // Mark the page directory page read-only
  thread_t * thread = sched_get_current_thread();
  pde_t * current_page_dir = NULL;

  if (thread == NULL) {
    current_page_dir = kernel_page_dir;
  } else {
    task_t * task = thread->task;
    current_page_dir = task->page_dir;
  }
      
  // Mark new page directory as read-only
  // Required by Xen
  vmm_map_page(current_page_dir,
	       page_dir,
	       VMM_V2P(page_dir),
	       PGTAB_ATTRIB_USR_RO | PG_FLAG_PRESENT);

  /* XXX Optional -- just forcing a flush for testing */
  vmm_flush_mmu_update_queue();

  return page_dir;
}


void
vmm_free_pgdir(pde_t * pgdir)
{
  int i = 0;

  /* Zero out the low mem kernel text map */
  for (i = 0; i < num_kernel_text_pdes; i++) {
    pde_t * pde_ktext = VMM_GET_PDE(pgdir, 
				    (i << PGTAB_REGION_SHIFT));
    pde_ktext->value = 0;
  }
  
  
  for (i = 0; i < num_phys_mem_pdes; i++) {

    pde_t * pde_kvirt = VMM_GET_PDE(pgdir, 
				    VMM_P2V(i << PGTAB_REGION_SHIFT));
    
    pde_kvirt->value = 0;
  }
    
  for (; i < PGDIR_SIZE; i++) {
    void * pgtab  = (void *)(pgdir[i].value & PAGE_MASK);
    if (pgtab != NULL) {
      kdinfo("Freeing page table %p", pgtab);
      vmm_ppf_free(VMM_P2V(pgtab));
    }
  }
  kdinfo("Freeing page dir: %p", pgdir);
  vmm_ppf_free(pgdir);
}

/*
 * Takes  a page_dir address, and a
 * virtual address, and maps the page at the vaddr. 
 */
int
vmm_map_page(pde_t * page_dir,
	     void * vaddr,
	     void * ppf,
	     int flags)
{
  uint32_t val = 0;
  pte_t * pte_ma = NULL;
  pte_t * pte_pa = NULL;
  pte_t * pte = NULL;
  
  pde_t * pde = VMM_GET_PDE(page_dir,
			    vaddr);
  
  pde = (pde_t *)VMM_P2V(pde);
  
  pde_t * pde_pa = (pde_t *)VMM_V2P(pde);
  
  pde_t * pde_ma = (pde_t *)phys_to_mach(pde_pa);
  
  if (pde->value == 0) {

    /* We need to set up a page table 
     * Page tables are referenced twice:
     * 1. In the kernel virtual address map
     * as part of the kernel's global page map
     * 2. In the PDE, as a page table
     * We need to ensure that mapping 1 is read-only
     * before we can request mapping 2
     */
    void * pt_ppf = vmm_ppf_alloc();
    
    if (pt_ppf == NULL) {
      kdinfo("Unable to allocate page table");
      return -1;
    }

    /* The page table has to be zeroed out initially */
    memset(pt_ppf, 0, PAGE_SIZE);
    kdverbose("pt_ppf:%p", pt_ppf);
    kdverbose("pt_ppf_v2p:%p", VMM_V2P(pt_ppf));

    
    void * pt_ppf_ma = phys_to_mach(VMM_V2P(pt_ppf));

    /* First Mark this page read-only in the kernel
     * virtual map
     */
    pte_t * pt_ppf_pte_ma = VMM_GET_PTE(page_dir,
					pt_ppf);
    
    
    val = (PGTAB_ATTRIB_SU_RO | 
	   PG_FLAG_PRESENT | 
	   ((uint32_t)pt_ppf_ma & PAGE_MASK));
    
    
    vmm_queue_mmu_update((uint32_t)pt_ppf_pte_ma |
			 MMU_NORMAL_PT_UPDATE,
			 val);

    /* Now Add the Page Table Entry to the Page Directory */
    /* Set its access rights to USR_RW, to give the
     * most open access rights to the page group
     * Read-only access is set at page-level
     */
    val = (PGTAB_ATTRIB_USR_RW | 
	   PG_FLAG_PRESENT | 
	   ((uint32_t)pt_ppf_ma & PAGE_MASK));
    

    val &= (~PD_FLAG_RSVD);
    
    
    vmm_queue_mmu_update((uint32_t)pde_ma |
			 MMU_NORMAL_PT_UPDATE,
			 val);

    vmm_flush_mmu_update_queue();
  } 



  pte_ma = (pte_t *)VMM_GET_PTE(page_dir, 
				vaddr);
  
  pte_pa = (pte_t *)mach_to_phys(pte_ma);
  
  pte = (pte_t *)VMM_P2V(pte_pa);

  val = 0;

  /* A NULL value means zero-mapping, we never
   * should map physical page '0'
   */
  if (ppf != NULL) {

    void * ppf_pa = VMM_V2P(ppf);
  
    void * ppf_ma = phys_to_mach(ppf_pa);
  
    kdverbose("pte_ppf    :%p", pte_pa);
    kdverbose("pte_ppf_p2v:%p", pte);
 
 
    kdverbose("ppf        :%p", ppf);
    kdverbose("ppf_pa        :%p", ppf_pa);
    kdverbose("ppf_ma    :%p", ppf_ma);
  
    // Replace previous mapping
    val = (flags | (uint32_t)ppf_ma);
  }

  if (pte->value != val) {

    vmm_queue_mmu_update((uint32_t)pte_ma | 
			 MMU_NORMAL_PT_UPDATE,
			 val);
  }
  
  return 0;
}

int
vmm_map_pages(pde_t * page_dir,
	      void * vaddr,
	      void * ppf,
	      int num_pages,
	      int flags)
{
  int i = 0;
  for (i = 0; i <  num_pages; i++) {
    if (vmm_map_page(page_dir,
		     vaddr + (i << PAGE_SHIFT),
		     ppf + (i << PAGE_SHIFT),
		     flags) < 0) {
      return -1;
    }
  }

  vmm_flush_mmu_update_queue();

  return 0;
}

uint32_t
vmm_get_mapping(pde_t * page_dir,
		void * vaddr)
{
  pte_t * pte_ma = NULL;
  pte_t * pte_pa = NULL;
  pte_t * pte = NULL;

  pde_t * pde = VMM_GET_PDE(page_dir,
			    vaddr);
  
  pde = (pde_t *)VMM_P2V(pde);
  
  if (pde->value == 0)  {
    /* The page table does not exist */
    kdtrace("No page table");
    return 0;
  }
  
  /* Get Machine Address of PTE  */
  pte_ma = VMM_GET_PTE(page_dir, vaddr);

  if (pte_ma  == NULL) {
    return 0;
  }

  /* Pseudo Physical Address of PTE */
  pte_pa = (pte_t *)mach_to_phys(pte_ma);

  /* Virtual Address of PTE */
  pte = (pte_t *)VMM_P2V(pte_pa);
  
  /* Machine Address of PPF + Flags */
  uint32_t mapping_ma = pte->value;

  /* PPF Machine Address */
  uint32_t ppf_ma = mapping_ma & PAGE_MASK;

  /* PPF Flags */
  uint32_t mapping_flags = mapping_ma & PAGE_DIV;

  /* PPF Pseudo Physical Address */
  uint32_t ppf_pa = 0;

  /* PA is only meaningful if MA is non-zero,
   * An MA of zero indicates a NULL mapping
   */
  if (ppf_ma !=  0) {
    ppf_pa = (uint32_t)mach_to_phys((void *)ppf_ma);
  }

  /* Mapping is it should look on unparavirted kernel */
  uint32_t mapping = ppf_pa | mapping_flags;
  
  return mapping;
}


/*
 * Takes  a page_dir address, and a
 * virtual address, and maps the page at the vaddr.
 * This variant used during kernel booting, all
 * address arguments are literal addresses, so
 * no conversion is necessary.
 */
void
vmm_init_map_page(pde_t * page_dir,
		  void * vaddr,
		  void * ppf,
		  int flags)
{
  pte_t * pte_ma = NULL;
  pte_t * pte_pa = NULL;
  
  pte_t new_pte;

  /* Get PTE Machine address */
  pte_ma = (pte_t *)VMM_GET_PTE(page_dir, 
				vaddr);
  
  assert(pte_ma != NULL);

  /* Get Machine page for the physical page */
  void * ppf_ma = phys_to_mach(ppf);
  
  /* Get Machine address for the PPF's PTE */
  pte_t * ppf_pte_ma = (pte_t *)VMM_GET_PTE(page_dir,
					    ppf);

  pte_t * ppf_pte_pa = NULL;

  if (ppf_pte_ma != NULL) {
    /* A page table entry for this ppf exists in the
     * default page table, add it to the PTE
     */
    ppf_pte_pa = (pte_t *)mach_to_phys(ppf_pte_ma);
  }

  if (ppf_pte_ma != NULL && 
      ppf_pte_pa->value != 0) {
    /* Xen default mapping existed, so use it */

    new_pte.value = ppf_pte_pa->value;
  } else {
    /* The PPF was unmapped in the Xen default mapping,
     * create a new mapping for it
     */
    
    /* Get pseudo-phys address for PTE */
    pte_pa = (pte_t *)mach_to_phys(pte_ma);
    
    /* Set the base value */
    new_pte.value = pte_pa->value;
    
    if (new_pte.value != 0) {
      return;
    }
        
    // Replace previous mapping
    new_pte.value = (flags | (uint32_t)(ppf_ma));
  }

  vmm_queue_mmu_update((uint32_t)pte_ma | MMU_NORMAL_PT_UPDATE,
		       new_pte.value);
  
}

void
vmm_init_map_pages(pde_t * page_dir,
	      void * vaddr,
	      void * ppf,
	      int num_pages,
	      int flags)
{
  int i = 0;
  for (i = 0; i <  num_pages; i++) {
    vmm_init_map_page(page_dir,
		      vaddr + (i << PAGE_SHIFT),
		      ppf + (i << PAGE_SHIFT),
		      flags);
  }

  vmm_flush_mmu_update_queue();
  
}



void
vmm_set_user_pgdir(void * page_dir)
{
  // Adjust address from our displaced
  // kernel mapping address.
  /*
  set_cr3((uint32_t)VMM_V2P(page_dir));
  
  uint32_t cr4 = 0;
 
  cr4 |= CR4_PGE;
  cr4 &= ~CR4_PAE;
  cr4 &= ~CR4_PSE;
  cr4 &= ~CR4_MCE;
  
  set_cr4(cr4);

  // Turn Paging on
  uint32_t cr0 = get_cr0();

  cr0 |= CR0_PG;

  set_cr0(cr0);
  */

  vmm_flush_mmu_update_queue();

  int count = 1;
  int succ_count = -1;
  int ret = 0;
  mmuext_op_t op;


  op.cmd = MMUEXT_NEW_BASEPTR;

  /* Xen needs a Machine Frame *Number * */
  op.arg1.mfn = (uint32_t)phys_to_mach(VMM_V2P(page_dir));
  op.arg1.mfn >>= PAGE_SHIFT;

  ret = HYPERVISOR_mmuext_op(&op, 
			     count, 
			     &succ_count, 
			     DOMID_SELF);

  assert(ret == 0);

  assert(succ_count == count);
  
}

/*
 * Sets CR3 to the kernel page directory,
 * and enables paging
 */
void
vmm_set_kernel_pgdir(void)
{
  // Set the kernel page directory
  /*
  set_cr3((uint32_t)VMM_V2P(kernel_page_dir));
  
  uint32_t cr4 = 0;
  
  cr4 |= CR4_PGE;
  cr4 &= ~CR4_PAE;
  cr4 &= ~CR4_PSE;
  cr4 &= ~CR4_MCE;
  
  set_cr4(cr4);

  // Turn Paging on
  uint32_t cr0 = get_cr0();

  cr0 |= CR0_PG;

  set_cr0(cr0);
  */

  vmm_flush_mmu_update_queue();

  int count = 1;
  int succ_count = -1;
  int ret = 0;
  mmuext_op_t op;

  op.cmd = MMUEXT_NEW_BASEPTR;

  op.arg1.mfn = (uint32_t)phys_to_mach(VMM_V2P(kernel_page_dir));

  op.arg1.mfn >>= PAGE_SHIFT;

  ret = HYPERVISOR_mmuext_op(&op, 
			     count, 
			     &succ_count, 
			     DOMID_SELF);

  assert(ret == 0);

  assert(succ_count == count);



  return;
}


pde_t *
vmm_create_kernel_pgdir()
{
  // Allocate the initial page directory
  init_page_count = 0;
  pde_t * page_dir = vmm_init_ppf_alloc();
  memset(page_dir, 0, PAGE_SIZE);
  
  kdtrace("Init Page Dir: %p",
	  page_dir);

  uint32_t i = 0;

  kdinfo("Phys Mem PPFs: %d", num_phys_mem_ppf);
  kdinfo("Phys Mem PDEs: %d", num_phys_mem_pdes);
  kdinfo("Ktext PPFs   : %d", num_kernel_text_ppf);
  kdinfo("Ktext PDEs   : %d", num_kernel_text_pdes);

  kdtrace("About to map Kernel text segment in low memory");
  
  // Allocate kernel page tables.
  // Has to be done without the buddy allocator, since the page 
  // allocator is not enabled
  // at this point.

  // First map only the kernel text segment in lower memory
  for (i = 0; i < num_kernel_text_pdes; i++) {
    pde_t * pde_ktext = VMM_GET_PDE(page_dir, 
				    (i << PGTAB_REGION_SHIFT));

    kdverbose("pde_ktext:%p", 
	      pde_ktext);
    
    void * pt_ppf = vmm_init_ppf_alloc();
    kdverbose("pt: %p", pt_ppf);
    memset(pt_ppf, 0, PAGE_SIZE); 
    
    pde_ktext->value = (PGTAB_ATTRIB_SU_RW | 
		  PG_FLAG_PRESENT | 
		  ((uint32_t)pt_ppf & PAGE_MASK));
    
    pde_ktext->value &= (~PD_FLAG_RSVD);

  }
  
  kdtrace("Mapping pages");

  // Map the kernel text segment in lower memory area
  // of the virtual memory map
  vmm_init_map_pages(page_dir,
		     0,
		     0,
		     num_kernel_text_ppf,
		     PGTAB_ATTRIB_SU_RW);

  kdtrace("About to map physical memory at %p",
	  (void *)VMM_KERNEL_BASE);

  // Map the entire physical memory in high memory area
  // of the virtual memory map.
  for (i = 0; i < num_phys_mem_pdes; i++) {


    pde_t * pde_kvirt = VMM_GET_PDE(page_dir, 
				    VMM_P2V(i << PGTAB_REGION_SHIFT));

    kdverbose("pde_kvirt:%p", 
	      pde_kvirt);

    // Allocate a page table
    void * pt_ppf = vmm_init_ppf_alloc();
    kdverbose("pt: %p", pt_ppf);
    memset(pt_ppf, 0, PAGE_SIZE); 
    
    pde_kvirt->value = (PGTAB_ATTRIB_SU_RW | 
			PG_FLAG_PRESENT | 
			((uint32_t)pt_ppf & PAGE_MASK));
    
    pde_kvirt->value &= (~PD_FLAG_RSVD);
    
  }
  
  kdtrace("Mapping pages");
  // Now map the physical memory
  // directly into virtual address range
  // starting at VMM_KERNEL_BASE
  vmm_init_map_pages(page_dir,
		     (void *)(VMM_KERNEL_BASE),
		     0,
		     num_phys_mem_ppf,
		     PGTAB_ATTRIB_SU_RW);
  
  kdtrace("init_page_count: %d",
	  init_page_count);
  
  return VMM_P2V(page_dir);
}

void
vmm_xen_map_pt(pte_t * pgtab, void * vaddr)
{
  int i = 0;

  void * max_addr = VMM_P2V((num_phys_mem_ppf << PAGE_SHIFT));

  for (; i < 1024; i++) {
    pte_t * pte = pgtab + i;

    void * va = (void *)((uint32_t)vaddr + (i << PAGE_SHIFT));

    void * pa = VMM_V2P(va);

    if (va >= max_addr) {
      break;
    }

    void * ma = phys_to_mach(pa);

    pte->value = (PGTAB_ATTRIB_SU_RW | 
		  PG_FLAG_PRESENT | 
		  ((uint32_t)ma & PAGE_MASK));
    
    pte->value &= (~PD_FLAG_RSVD);
  }
  

}

pde_t *
vmm_xen_create_kernel_pgdir()
{
 
  // Allocate the initial page directory
  init_page_count = 0;

  pde_t * page_dir = kernel_page_dir;

  //pde_t * page_dir = vmm_init_ppf_alloc();
  //memset(page_dir, 0, PAGE_SIZE);
  
  kdtrace("Init Page Dir: %p",
	  page_dir);

  uint32_t i = 0;

  kdinfo("Phys Mem PPFs: %d", num_phys_mem_ppf);
  kdinfo("Phys Mem PDEs: %d", num_phys_mem_pdes);
  kdinfo("Ktext PPFs   : %d", num_kernel_text_ppf);
  kdinfo("Ktext PDEs   : %d", num_kernel_text_pdes);

  kdtrace("About to map physical memory at %p",
	  (void *)VMM_KERNEL_BASE);

  // Map the entire physical memory in high memory area
  // of the virtual memory map.

  for (i = 0; i < num_phys_mem_pdes; i++) {

    void * vaddr = VMM_P2V(i << PGTAB_REGION_SHIFT);

    pde_t * pde_kvirt = VMM_GET_PDE(page_dir, 
				    vaddr);

    kdverbose("pde_kvirt:%p", 
	      pde_kvirt);

    // Allocate a page table
    void * pt_ppf = vmm_init_ppf_alloc();
    kdverbose("pt: %p", pt_ppf);
    memset(pt_ppf, 0, PAGE_SIZE); 

    void * pt_ppf_ma = phys_to_mach(pt_ppf);
    
    pte_t * pt_pte = VMM_GET_PTE(page_dir,
				 pt_ppf);
    

    pte_t pteval;
    
    //pteval.value = pt_pte->value;

    pteval.value = (PGTAB_ATTRIB_SU_RO | 
		    PG_FLAG_PRESENT | 
		    ((uint32_t)pt_ppf_ma & PAGE_MASK));

    /* First mark the page table Read-only */
    vmm_queue_mmu_update((uint32_t)pt_pte | MMU_NORMAL_PT_UPDATE,
			 pteval.value);

    
    mmu_update_t req;

    uint32_t ptr = (uint32_t)phys_to_mach(pde_kvirt);

    req.ptr = ptr | MMU_NORMAL_PT_UPDATE;
  
    req.val = (PGTAB_ATTRIB_USR_RW | 
	       PG_FLAG_PRESENT | 
	       ((uint32_t)pt_ppf_ma & PAGE_MASK));
    
    req.val &= (~PD_FLAG_RSVD);

    /* Now add the new page table to the page directory */
    vmm_queue_mmu_update(req.ptr,
			 req.val);
        

    /*
    pde_kvirt->value = (PGTAB_ATTRIB_SU_RW | 
			PG_FLAG_PRESENT | 
			((uint32_t)pt_ppf & PAGE_MASK));
    
    pde_kvirt->value &= (~PD_FLAG_RSVD);
    */
  }
  
  vmm_flush_mmu_update_queue();

  kdtrace("Mapping pages");
  // Now map the physical memory
  // directly into virtual address range
  // starting at VMM_KERNEL_BASE
  
  vmm_init_map_pages(page_dir,
		     (void *)(VMM_KERNEL_BASE),
		     0,
		     num_phys_mem_ppf,
		     PGTAB_ATTRIB_SU_RW);
  
  kdtrace("init_page_count: %d",
	  init_page_count);
  
  return VMM_P2V(page_dir);
}


/* initialize the Xen page map */
int
vmm_xen_page_init()
{ 

  assert(xen_start_info != NULL);
  
  kernel_page_dir = (void *)xen_start_info->pt_base;

  assert(kernel_page_dir != NULL);

  xen_mfn_list = (void **)xen_start_info->mfn_list;
  
  assert(xen_mfn_list != NULL);

  
  return 0;
} 

shared_info_t *
vmm_xen_map_shared_info()
{
  /* Allocate some arbitrary 
   * page, this page will be used
   * as shared_info
   */
  void * ppf_va = vmm_ppf_alloc();
  
  assert(ppf_va != NULL);

  pte_t * pte_ma = VMM_GET_PTE(kernel_page_dir,
			       ppf_va);
  
  uint32_t shinfo_ma = xen_start_info->shared_info;

  uint32_t val = (PGTAB_ATTRIB_SU_RW |
		  PG_FLAG_PRESENT |
		  shinfo_ma);
  
  vmm_queue_mmu_update((uint32_t)pte_ma | MMU_NORMAL_PT_UPDATE,
		       val);
  
  vmm_flush_mmu_update_queue();
  
  return (shared_info_t *)ppf_va;
}


void
vmm_page_init()
{
  num_phys_mem_ppf = machine_phys_frames();
  num_kernel_text_ppf = (USER_MEM_START >> PAGE_SHIFT);
  num_kernel_text_pdes = VMM_NUM_PDES(num_kernel_text_ppf);
  num_phys_mem_pdes = VMM_NUM_PDES(num_phys_mem_ppf);

  kdinfo("Physical page frames   : 0x%x", num_phys_mem_ppf);

  kdinfo("About to create Xen mappings");

  vmm_xen_page_init();


  kdtrace("About to create kernel page directory");

  kernel_page_dir  = vmm_xen_create_kernel_pgdir();
  

  // Add the pages used for building the initial kernel
  // page directory to the pages consumed by kernel
  // text segment
  unsigned int kernel_pages = ((USER_MEM_START >> PAGE_SHIFT) + 
			       init_page_count);
  
  kdtrace("Kernel Pages: bin:0x%x + init_pgdir:0x%x",
	  (USER_MEM_START >> PAGE_SHIFT),
	  init_page_count);
  
  // Set the user memory area start address
  void * lomem_free_start = VMM_P2V((kernel_pages << PAGE_SHIFT));

  // Find the size of user memory area in pages
  unsigned int lomem_free_pages = (num_phys_mem_ppf - 
				   kernel_pages - 
				   VMM_KERNEL_STACK_PAGES);
  
  kdtrace("lomem_start:%p, lomem_pgs: %d",
	  lomem_free_start,
	  lomem_free_pages);

  // Set the high memory area startt address.
  void * highmem_free_start = VMM_P2V(((num_phys_mem_ppf - 
					VMM_KERNEL_STACK_PAGES) << 
				       PAGE_SHIFT));

  kdtrace("himem_start:%p, himem_pgs: %d",
	  highmem_free_start,
	  VMM_KERNEL_STACK_PAGES);
  
  // Set these pointer values so they can be used directly
  // by the allocation routines, without worrying about adjusting
  // to the VMM_KERNEL_BASE address. 
  lomem_zone_ptr = (vmm_zone_t *)VMM_P2V(&lomem_zone);

  himem_zone_ptr = (vmm_zone_t *)VMM_P2V(&himem_zone);

  kdtrace("About to initialize Buddy Page Allocator");

  // initialize Buddy allocator for user memory
  // and high memory areas.
  vmm_zone_init(lomem_zone_ptr, 
		lomem_free_start,
		lomem_free_pages);
  
  vmm_zone_init(himem_zone_ptr,
		highmem_free_start,
		VMM_KERNEL_STACK_PAGES);
  
  kdtrace("Finsihed VMM Paging Initialization");


  /* We need memory manager initialized before we
   * map xen_shared_info
   */
  
  xen_shared_info = (shared_info_t *)vmm_xen_map_shared_info();

  assert(xen_shared_info != NULL);


}
