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
#include <vmm_page.h>
#include <common_kern.h>
#include <kdebug.h>
#include <kernel.h>
#include <vmm_zone.h>

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
  void * ppf = (void *)(USER_MEM_START + (init_page_count << PAGE_SHIFT)); 

  /* XXX Page allocation should not reset memory */
  /** If out of pages, and a user request, yield to other process
   */
  /** If a kernel request, something bad has happened */
  init_page_count++;
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
  return 0;
}

uint32_t
vmm_get_mapping(pde_t * page_dir,
		void * vaddr)
{
  pte_t * pte = NULL;
  pde_t * pde = VMM_GET_PDE(page_dir,
			    vaddr);
  
  pde = (pde_t *)VMM_P2V(pde);

  if (pde->value == 0)  {
    /* The page table does not exist */
    kdtrace("No page table");
    return 0;
  }
  
  pde->value &= ~PD_FLAG_RSVD;

  pte = VMM_GET_PTE(page_dir, vaddr);

  pte = (pte_t *)VMM_P2V(pte);
  
  return pte->value;
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
  pte_t * pte = NULL;
  pde_t * pde = VMM_GET_PDE(page_dir,
			    vaddr);
  
  // Only toggle the flags, replacing with newer attributes
  pde->value = (flags | PG_FLAG_PRESENT | (uint32_t)(pde->value & PAGE_MASK));
  pde->value &= (~PD_FLAG_RSVD);

  
  pte = (pte_t *)VMM_GET_PTE(page_dir, 
			     vaddr);
    
  // Replace previous mapping
  pte->value = (flags | (uint32_t)(ppf));
 
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
  pte_t * pte = NULL;
  pde_t * pde = VMM_GET_PDE(page_dir,
			    vaddr);

  pde = (pde_t *)VMM_P2V(pde);

  if (pde->value == 0) {
    // Allocate a page table
    void * pt_ppf = vmm_ppf_alloc();
    
    if (pt_ppf == NULL) {
      kdinfo("Unable to allocate page table");
      return -1;
    }

    memset(pt_ppf, 0, PAGE_SIZE);
    kdverbose("pt_ppf:%p", pt_ppf);
    kdverbose("pt_ppf_v2p:%p", VMM_V2P(pt_ppf));
    pde->value = (flags | PG_FLAG_PRESENT | ((uint32_t)VMM_V2P(pt_ppf) & 
					     PAGE_MASK));
    pde->value &= (~PD_FLAG_RSVD);
  } else {
    // Only toggle the flags, replacing with newer attributes
    pde->value = (flags | PG_FLAG_PRESENT | (uint32_t)(pde->value & PAGE_MASK));
    pde->value &= (~PD_FLAG_RSVD);
  }

  pte = (pte_t *)VMM_GET_PTE(page_dir, 
			     vaddr);
  
  kdverbose("pte_ppf    :%p", pte);
  kdverbose("pte_ppf_p2v:%p", VMM_P2V(pte));
 
  pte = (pte_t *)VMM_P2V(pte);
 
  kdverbose("ppf        :%p", ppf);
  kdverbose("ppf_v2p    :%p", VMM_V2P(ppf));
  
  // Replace previous mapping
  pte->value = (flags | (uint32_t)VMM_V2P(ppf));

  return 0;
}


void
vmm_set_user_pgdir(void * page_dir)
{
  // Adjust address from our displaced
  // kernel mapping address.
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

}

/*
 * Sets CR3 to the kernel page directory,
 * and enables paging
 */
void
vmm_set_kernel_pgdir(void)
{
  // Set the kernel page directory
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
vmm_page_init()
{
  num_phys_mem_ppf = machine_phys_frames();
  num_kernel_text_ppf = (USER_MEM_START >> PAGE_SHIFT);
  num_kernel_text_pdes = VMM_NUM_PDES(num_kernel_text_ppf);
  num_phys_mem_pdes = VMM_NUM_PDES(num_phys_mem_ppf);

  kdinfo("Physical page frames   : 0x%x", num_phys_mem_ppf);
  kdtrace("About to create kernel page directory");

  kernel_page_dir  = vmm_create_kernel_pgdir();

  kdtrace("Created Page tables, about to enable paging");
  vmm_set_kernel_pgdir();

  kdinfo("Set kernel page directory CR3");
  kdtrace("Paging enabled");
  
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

}
