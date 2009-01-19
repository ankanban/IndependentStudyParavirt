/** @file vmm_page.h 
 *
 *  @brief Page-level allocator declarations
 *
 *  @author Anshuman P.Kanetkar (apk)
 * 
 */
#ifndef __VMM_PAGE_H__
#define __VMM_PAGE_H__

#include <stdint.h>

/**
 * @brief A page directory entry
 */
typedef struct pde {
  unsigned int value;
}  __attribute__((packed)) pde_t;

/**
 * @brief A Page table entry
 */
typedef struct pte {
  unsigned int value;
} __attribute__((packed)) pte_t;


/*
 * The kernel uses a 3GB/1GB User/Kernel Split.
 * The user process can address slightly less than 3 GB of space
 * The lower 1 MB is used to map the Kernel text segment. 
 */

/* Start of physical memory mapping (>3GB in user virtual memory) */
#define VMM_KERNEL_BASE (0xc0000000UL)
#define VMM_KERNEL_MASK (0x3fffffffUL)

/* Virtual<->Physical address conversion */
#define VMM_V2P(vaddr) ((void *)((uint32_t)(vaddr) & VMM_KERNEL_MASK))
#define VMM_P2V(paddr) ((void *)((uint32_t)(paddr) | VMM_KERNEL_BASE))

/* Defined here as it has implications for memory allocation */
#define VMM_MAX_THREADS 4096UL
#define VMM_MAX_TASKS 4096UL
#define VMM_KERNEL_THREAD_STACK_PAGES 1UL
#define VMM_KERNEL_THREAD_STACK_SIZE (VMM_KERNEL_THREAD_STACK_PAGES << PAGE_SHIFT)

#define VMM_KERNEL_STACK_PAGES (VMM_MAX_THREADS * VMM_KERNEL_THREAD_STACK_PAGES)

#define VMM_KERNEL_STACK_SPACE (VMM_KERNEL_STACK_PAGES << PAGE_SHIFT)

/* Page manipulation macros */
#define PAGE_DIV (PAGE_SIZE -1)
#define PAGE_MASK ~(PAGE_SIZE - 1)

/* Page directory/table attribute flag combinations */

#define PGDIR_ATTRIB_SU_RO 0x001UL
#define PGDIR_ATTRIB_SU_RW 0x003UL

#define PGDIR_ATTRIB_USR_RO 0x005UL
#define PGDIR_ATTRIB_USR_RW 0x007UL

#define PGTAB_ATTRIB_SU_RO 0x001UL
#define PGTAB_ATTRIB_SU_RW 0x003UL

#define PGTAB_ATTRIB_USR_RO 0x005UL
#define PGTAB_ATTRIB_USR_RW 0x007UL

/* Page directory/table entry flag bits */
#define PG_FLAG_PRESENT 0x1UL
#define PG_FLAG_WR      0x2UL
#define PG_FLAG_USR     0x4UL
#define PG_FLAG_PWT     0x8UL
#define PG_FLAG_PCD     0x10UL
#define PG_FLAG_ACC     0x20UL
#define PG_FLAG_DTY     0x40UL

#define PD_FLAG_RSVD    0x40UL

/* Page directory/table manipulation macros */

#define PDE_SHIFT 2
#define PGDIR_SIZE (PAGE_SIZE >> PDE_SHIFT)
#define PGDIR_SHIFT (PAGE_SHIFT - PDE_SHIFT)


#define PTE_SHIFT 2
#define PGTAB_SIZE (PAGE_SIZE >> PTE_SHIFT)
#define PGTAB_SHIFT (PAGE_SHIFT - PTE_SHIFT)


/* Sizes covered by a page table struct */
#define PGTAB_REGION_SHIFT (PGTAB_SHIFT + PAGE_SHIFT)  
#define PGTAB_REGION_SIZE (1 << PGTAB_REGION_SHIFT)
#define PGTAB_REGION_DIV (PGTAB_REGION_SIZE - 1)
#define PGTAB_REGION_MASK (~PGTAB_REGION_DIV)


#define PDE_ENTRY_MASK 0xffc00000
#define PDE_ENTRY_SHIFT 22
#define PTE_ENTRY_MASK 0x003ff000
#define PTE_ENTRY_SHIFT 12

/* Given a page frame number, get its physical address */
#define VMM_PPN2PPF(page_num) ((page_num) << PAGE_SHIFT)

/* Find the number of PDEs a given number of Page frames 
 * would consume
 */
#define VMM_NUM_PDES(num_pages) ((VMM_PPN2PPF((num_pages) - 1)	\
				  >> PDE_ENTRY_SHIFT) + 1)

// Returns a ptr to the page directory entry for teh given virtual address
#define VMM_GET_PDE(pgdir, vaddr) ((pde_t *)(pgdir) + (((uint32_t)(vaddr) & \
						      PDE_ENTRY_MASK) >> \
						     PDE_ENTRY_SHIFT))

// Returns a ptr to the page tablefor the given virtual address
#define VMM_GET_PGTAB(pgdir, vaddr) ((pte_t *)(VMM_GET_PDE(pgdir,	   \
							   vaddr)->value & \
					       PAGE_MASK))

// Returns a ptr to the page tabel entry for the given virtual address
#define VMM_GET_PTE(pgdir, vaddr)				  \
  ((pte_t *)(							  \
	     (VMM_GET_PGTAB(pgdir, vaddr) == NULL)?		  \
	     (NULL):						  \
	     (VMM_GET_PGTAB(pgdir, vaddr) +			  \
	      (((uint32_t)vaddr &				  \
		PTE_ENTRY_MASK) >>				  \
	       PTE_ENTRY_SHIFT))))

// Returns the physical page frame for a given virtual address
#define VMM_GET_PPF(pgdir, vaddr)  ((void *)(VMM_GET_PTE(pgdir,		\
							 vaddr)->value & \
					     PAGE_MASK))

/* get the page address for a given address */
#define VMM_PAGE_ADDR(vaddr) ((uint32_t)(vaddr) & PAGE_MASK)
/* get the last page in a given memory region */
#define VMM_END_PAGE_ADDR(vaddr, size) ((uint32_t)(vaddr + size - 1) & PAGE_MASK)
/* The size of a memory region in pages */
#define VMM_NUM_PAGES(vaddr, size)		\
(((VMM_END_PAGE_ADDR(vaddr) - VM_PAGE_ADDR(vaddr)) >> PAGE_SHIFT) + 1)

/* num'th page from a given start address */
#define VMM_VMA_PAGE(start, num)  (VMM_PAGE_ADDR(start) + ((num) << PAGE_SHIFT))  
int 
vmm_floor_log2(unsigned int n);

/**
 * @brief Allocate a page in low memory using the buddy allocator
 */
void *
vmm_ppf_alloc(void);

/**
 * @brief Release a page in low memory using the buddy allocator
 */
void
vmm_ppf_free(void *);

/**
 * @brief Allocate contiguous pages in low memory
 * @param num_pages Number of pages to allocate
 * internally rounded off (ceil) to nearest power of 2
 */
void *
vmm_ppfs_alloc(int num_pages);

/**
 * @brief Free low memory pages
 * @param pgaddr Kernel virtual address of page
 * @param num_pages Number of pages, as used in vmm_ppfs_alloc
 */
void 
vmm_ppfs_free(void *pgaddr, int num_pages);

/**
 * @brief PAge allocator for high memory. 
 */
void *
vmm_ppf_highmem_alloc(void);

/**
 * @brief Free a high memory page
 */
void
vmm_ppf_highmem_free(void * pgaddr);

/**
 * @brief Allocate contiguous pages in high memory
 */
void *
vmm_ppfs_highmem_alloc(int num_pages);

/**
 * @brief Free contiguously allocated pages in high memory
 */
void 
vmm_ppfs_highmem_free(void *pgaddr, 
		      int num_pages);

/**
 * @brief Returns the page table entry for page containing the
 * given virtual address
 * @param page_dir Kernel virtual address of page directory to use
 * @param vaddr The virtual address
 */
uint32_t
vmm_get_mapping(pde_t * page_dir,
		void * vaddr);

/**
 * @brief Map a virtual memory region with physical page frames
 * starting at the given kernel virtual page address
 * @param page_dir Page directory to use (kernel virtual addr)
 * @param vaddr page address of the virtual memory area
 * @param ppf kernel virtual address of the starting page frame
 * @param flags attributes to be set in the page's page table entry 
 */
int
vmm_map_pages(pde_t * page_dir,
	      void * vaddr,
	      void * ppf,
	      int num_pages,
	      int flags);

/**
 * @brief Map a virtual memory page with physical page frame
 * @param page_dir Page directory to use (kernel virtual addr)
 * @param vaddr kernel virtual memory page address
 * @param ppf kernel virtual address of the page frame
 * @param flags attributes to be set in the page's page table entry 
 */
int
vmm_map_page(pde_t * page_dir,
	     void * vaddr,
	     void * ppf,
	     int flags);

/**
 * @brief Allocates and initializes a user process' page directory
 */
pde_t *
vmm_create_pgdir(void);

/**
 * @brief Frees an allocated page directory, also frees any page tables
 * allocated by the process that used this page directory
 * @param pgdir Page directory.
 */
void
vmm_free_pgdir(pde_t * pgdir);

/**
 * @brief Intialize paging subsystem, allocators and other state
 */
void
vmm_page_init();

/**
 * @brief Set the current page directory (register CR3) to the given
 * page directory. 
 * @param page_dir Kernel virtual or physical address of page directory
 */
void
vmm_set_user_pgdir(void * page_dir);

/**
 * @brief Sets the kernel page directory from the kernel_page_dir global
 * variable.
 */
void
vmm_set_kernel_pgdir(void);


#endif /* __VMM_PAGE_H__ */
