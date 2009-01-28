/** @file vm_area.c
 *
 *  @brief Virtual Memory region manipulation
 *
 *  @author Anshuman P.Kanetkar (apk)
 * 
 */
//#define FORCE_DEBUG
#define DEBUG_LEVEL KDBG_INFO

#include <task.h>
#include <thread.h>
#include <vm_area.h>
#include <string.h>
#include <elf_410.h>
#include <variable_queue.h>
#include <assert.h>
#include <stdlib.h>
#include <kdebug.h>

char * vma_types[] = {
  "kstk",
  "dir",
  "alloc",
  "zpage",
  "cow",
  "copy",
  "file"
};

const char *
vma_get_type(int type)
{
  return vma_types[type];
}


int
vm_area_create_identity(vm_area_t * vma)
{
  task_t * task = vma->task;
  uint32_t flags = vma->flags;
  uint32_t size = vma->size;
  void * start_virt = vma->start_address;
  void * start_phys = vma->src.paddr;
  
  void * start_vpf = (void *)((uint32_t)start_virt & PAGE_MASK);
  void * end_vpf = (void *)(((uint32_t)start_virt + size - 1) & PAGE_MASK);
  void * start_ppf = (void *)((uint32_t)start_phys & PAGE_MASK);
  
  void * vpf = NULL;
  void * ppf = NULL;
  int i = 0;
  int num_pages = ((uint32_t)((char *)end_vpf - (char *)start_vpf) >> PAGE_SHIFT) + 1;
  
  kdinfo("identity spage: %p, epage: %p, vmarealen: 0x%x, num_pages: 0x%x",
	 start_vpf,
	 end_vpf,
	 size, 
	 num_pages);

  for (i = 0; i < num_pages; i++) {
    vpf = (char *)start_vpf + (i << PAGE_SHIFT);
    ppf = (char *)start_ppf + (i << PAGE_SHIFT);

    if (vmm_map_page(task->page_dir,
		     vpf,
		     ppf,
		     flags) < 0) {
      return -1;
    }
    
    kdverbose("vmarea_ident: N task %d, vpf:%p, ppf:%p, flags: %0x%x",
	      task->id,
	      vpf,
	      ppf,
	      flags);
     
  }
  
  vmm_flush_mmu_update_queue();

  return 0;
}

/*
 * Copies the mapping from the given src vma
 * sets the flags for both mappings as specified
 * For the source, a 'mask' is provided that has
 * zeroes for bits to suppress in the attributes.
 */
int
vm_area_copy_pagemap(vm_area_t * dst_vma,
		     uint32_t dst_flags,
		     uint32_t src_mask)
{
  vm_area_t * src_vma = dst_vma->src.vma;
  task_t * dst_task = dst_vma->task;
  task_t * src_task = src_vma->task;

  void * start = src_vma->start_address;
  uint32_t size = src_vma->size;

  void * start_vpf = (void *)((uint32_t)start & PAGE_MASK);
  void * end_vpf = (void *)(((uint32_t)start + size - 1) & PAGE_MASK);
  void * vpf = NULL;

  int i = 0;
  int num_pages = ((uint32_t)((char *)end_vpf - (char *)start_vpf) >> PAGE_SHIFT) + 1;

  kdinfo("vmarealen: %u, pages: %u",
	 size, 
	 num_pages);
  
  if (dst_vma->vma_flags & VMA_FLAG_STACK) {
    kdtrace("Initializing COW map for stack");
  }

  uint32_t mapping = 0;
  
  for (i = 0; i < num_pages; i++) {

    vpf = (char *)start_vpf + (i << PAGE_SHIFT);

    mapping = vmm_get_mapping(src_task->page_dir, 
			      vpf);

    void * ppf = (void *)(mapping & PAGE_MASK);
    
    dst_flags |= mapping & PG_FLAG_PRESENT;

    if (vmm_map_page(dst_task->page_dir,
		     vpf,
		     ppf,
		     dst_flags) < 0) {
      kdinfo("Unable to copy pagemap");
      return -1;
    }
    
    uint32_t orig_flags = (mapping & ~PAGE_MASK);
    
    /* This should not fail */
    assert (vmm_map_page(src_task->page_dir,
			 vpf,
			 ppf,
			 orig_flags & src_mask) < 0);
    
  }
    
  vmm_flush_mmu_update_queue();

  return 0;
}

/* Unused */
void
vm_area_copy_pagemap_single(vm_area_t * vma_dst,
			    void * vaddr,
			    uint32_t dst_flags,
			    uint32_t src_mask)
{
  vm_area_t * vma_src = vma_dst->src.vma;
  task_t * task = vma_dst->task;
  task_t * task_src = vma_src->task;
  void * vpf = (void *)((uint32_t)vaddr & PAGE_MASK);

  uint32_t mapping = vmm_get_mapping(task_src->page_dir, 
				     vpf);
  
  void * ppf = (void *)(mapping & PAGE_MASK);
  
  kdinfo("vmarea cpsingle: vpf: %p, ppf: %p",
	 vpf,
	 ppf);
  
  vmm_map_page(task->page_dir,
	       vpf,
	       ppf,
	       dst_flags);
  
  uint32_t orig_flags = (mapping & ~PAGE_MASK);
  
  vmm_map_page(task_src->page_dir,
	       vpf,
	       ppf,
	       orig_flags & src_mask);

  vmm_flush_mmu_update_queue();

}

/**
 * Copies a VM area from the source VMA. Allocates a new
 * page for each destination virtual page address.
 */
int
vm_area_copy(vm_area_t * dst_vma,
	     void * dst_start,
	     uint32_t size)
{
  int i = 0;
  task_t * dst_task = dst_vma->task;
  task_t * src_task = dst_vma->src.vma->task;

  void * dst_start_vpf = (void *)((uint32_t)dst_start & PAGE_MASK);
  void * dst_end_vpf = (void *)(((uint32_t)dst_start + size - 1) & PAGE_MASK);
  int num_pages = ((uint32_t)(dst_end_vpf - dst_start_vpf) >> PAGE_SHIFT) + 1;
  void * dst_vpf = NULL;
  uint32_t mapping = 0;

  kdinfo("copy on write for vmarea start: %p, len: %u, pages: %u",
	  dst_start,
	  size, 
	  num_pages);

  for (i = 0; i < num_pages; i++) {

    dst_vpf = dst_start_vpf + (i << PAGE_SHIFT);
    
    mapping = vmm_get_mapping(src_task->page_dir, 
			      dst_vpf);

    void * src_ppf = NULL;
    

    kdverbose("mapping = 0x%x", mapping);

    if ((mapping & PAGE_MASK) == 0) {      

      /* Set the page mapping in teh source VMA/task */
      if (vmm_map_page(dst_task->page_dir,
		       dst_vpf,
		       src_ppf,
		       (mapping & ~PAGE_MASK)) < 0) {
	return -1;
      }

      
    } else { 
      void * dst_ppf = vmm_ppf_alloc();
      src_ppf = VMM_P2V(mapping & PAGE_MASK);
      
      kdinfo("vmareacp: COPY task %d, vpf:%p, src_ppf:%p, dst_ppf:%p, flags: 0x%x",
	      dst_task->id,
       	      dst_vpf,
	      src_ppf,
	      dst_ppf,
	     (mapping & ~PAGE_MASK));
      
      if (vmm_map_page(dst_task->page_dir,
		       dst_vpf,
		       dst_ppf,
		       (mapping & ~PAGE_MASK)) < 0) {
	return -1;
      }
      
      kdinfo("vmareacp: Mapped new page");

      /* Copy to dest. ppf from source ppf */
      memcpy(dst_ppf, src_ppf, PAGE_SIZE);

      kdinfo("vmareacp: Copied to dst ppf");

    }
    
  }
  
  vmm_flush_mmu_update_queue();
  

  return 0;
}

/* 
 * Copies content from the src VMA into a new vma, 
 * with a fresh mapping. Unused.
 */
int
vm_area_copy_on_write(vm_area_t * vma,
		      void * dst_start,
		      uint32_t size)
{
  // for vma_area's
  int i = 0;
  task_t * task = vma->task;
  void * dst_start_vpf = (void *)((uint32_t)dst_start & PAGE_MASK);
  void * dst_end_vpf = (void *)(((uint32_t)dst_start + size - 1) & PAGE_MASK);
  int num_pages = ((uint32_t)(dst_end_vpf - dst_start_vpf) >> PAGE_SHIFT) + 1;
  void * dst_vpf = NULL;
  uint32_t mapping = 0;

  kdinfo("copy on write for vmarea start: %p, len: %u, pages: %u",
	 dst_start,
	 size, 
	 num_pages);

  for (i = 0; i < num_pages; i++) {

    dst_vpf = dst_start_vpf + (i << PAGE_SHIFT);
    
    mapping = vmm_get_mapping(task->page_dir, 
			      dst_vpf);
    
    kdverbose("mapping = 0x%x", mapping);

    if ((mapping & PAGE_MASK) == 0) {
      
      // Error, we can't have an unmapped page,
      // the COW source has to be present, since
      // we give priority to the source VMA, and
      // that should have loaded the page
      //panic("Found unmapped pte in a COW VMA");

      int rc = vma->src.vma->nopage(vma,
				    dst_vpf);
      if (rc < 0) {
	panic("Unable to map COW page");
      }
      
      mapping = vmm_get_mapping(task->page_dir, 
				dst_vpf);
      
      if ((mapping & PAGE_MASK) == 0) {
	panic("Invalid mapping, unable to map COW page");
      }
      
    } else { 
      void * dst_ppf = vmm_ppf_alloc();

      if (dst_ppf == NULL) {
	return -1;
      }

      void * src_ppf = VMM_P2V(mapping & PAGE_MASK);

      kdinfo("vmareacp: COW task %d, vpf:%p, src_ppf:%p, dst_ppf:%p, flags: 0x%x",
	      task->id,
       	      dst_vpf,
	      src_ppf,
	      dst_ppf,
	      vma->flags);
      
      if (vmm_map_page(task->page_dir,
		       dst_vpf,
		       dst_ppf,
		       vma->flags) < 0) {
	vmm_ppf_free(dst_ppf);
	return -1;
      }
      
      kdinfo("Mapped new page for write before copy");

      memcpy(dst_ppf, src_ppf, PAGE_SIZE);

      kdinfo("Copied to dst ppf");

      /*
       * To do: Free the source page if its count goes to 0,
       * this is better than having it around until the entire
       * physical page frame area is copied by all other threads.
       * This requires a per-page data structure which we don't have...yet
       */
    }
    
  }

  vmm_flush_mmu_update_queue();

  return 0;
}


/**
 * Unmap all the mappings for the given vma.
 * Also deallocate pages allocated for the vma.
 * Does not delete kernel stack pages.
 */
void
vm_area_unmap(vm_area_t * vma)
{
  int i = 0;
  int rc = 0;
  void * start = vma->start_address;
  unsigned int size = vma->size;
  task_t * task = vma->task;
  // for vma_area's
  void * start_vpf = (void *)((uint32_t)start & PAGE_MASK);
  void * end_vpf = (void *)(((uint32_t)start + size - 1) & PAGE_MASK);
  void * vpf = NULL;
  int num_pages = ((uint32_t)(end_vpf - start_vpf) >> PAGE_SHIFT) + 1;

  kdinfo("vma_unmap: vmarealen: %u, pages: %u",
	  size, 
	  num_pages);

  uint32_t mapping = 0;

  for (i = 0; i < num_pages; i++) {
    vpf = start_vpf + (i << PAGE_SHIFT);

    mapping = vmm_get_mapping(task->page_dir, 
			      vpf);

    void * page = (void *)(mapping & PAGE_MASK);
    
    if (page != NULL) {
      kdinfo("Releasing page: p:%p, v:%p, vma:%p",
	     page, 
	     VMM_P2V(page),
	     vma);
      
      /* Kernel stack memory is always released in a block
       * using vmm_ppfs_highmem_free
       */
      if (vma->type != VMA_TYPE_KERNEL_STACK) {
	    
	/* Update the page map 
	 * This should not fail, since
	 * we are releasing a page for which
	 * the page table was already allocated
	 */
	rc = vmm_map_page(task->page_dir,
			  vpf,
			  NULL,
			  0 );
	assert(rc == 0);
	
	/* Can't delay queue flush, because we have to
	 * free the page here */
	vmm_flush_mmu_update_queue();

	vmm_ppf_free(VMM_P2V(page));
      }
    }
  }


}

/** Create a null (0)mapping for the VMA region
 */
int
vm_area_create_null(vm_area_t * vma)
{
  int i = 0;
  void * start = vma->start_address;
  unsigned int size = vma->size;
  task_t * task = vma->task;
  // for vma_area's
  void * start_vpf = (void *)((uint32_t)start & PAGE_MASK);
  void * end_vpf = (void *)(((uint32_t)start + size - 1) & PAGE_MASK);
  void * vpf = NULL;
  int num_pages = ((uint32_t)(end_vpf - start_vpf) >> PAGE_SHIFT) + 1;

  kdinfo("vmarealen: %u, pages: %u",
	  size, 
	  num_pages);


  for (i = 0; i < num_pages; i++) {
    vpf = start_vpf + (i << PAGE_SHIFT);

    /* Update the page map with a NULL page */
    if (vmm_map_page(task->page_dir,
		     vpf,
		     NULL,
		     PG_FLAG_USR) < 0) {
      return -1;
    }


  }

  vmm_flush_mmu_update_queue();
  return 0;
}

int
vm_area_setflags(vm_area_t * vma,
		 void * start,
		 uint32_t size)
{
  int i = 0;
  task_t * task = vma->task;
  // for vma_area's
  void * start_vpf = (void *)((uint32_t)start & PAGE_MASK);
  void * end_vpf = (void *)(((uint32_t)start + size - 1) & PAGE_MASK);
  void * vpf = NULL;
  int num_pages = ((uint32_t)(end_vpf - start_vpf) >> PAGE_SHIFT) + 1;
  uint32_t mapping = 0;
  
  kdinfo("vmarealen: %u, pages: %u",
	 size, 
	 num_pages);

  for (i = 0; i < num_pages; i++) {
    vpf = start_vpf + (i << PAGE_SHIFT);

    
    
    mapping = vmm_get_mapping(task->page_dir, 
			      vpf);
    void * ppf = (void *)(mapping & PAGE_MASK);

    if (vmm_map_page(task->page_dir,
		     vpf,
		     ppf,
		     vma->flags) < 0) {
      return -1;
    }
    
    kdinfo("vmarea: M task %d, vpf:%p, ppf:%p, flags: 0x%x",
	   task->id,
	   vpf,
	   ppf,
	   vma->flags);

  }

  vmm_flush_mmu_update_queue();

  return 0;
}

int
vm_area_create(vm_area_t * vma,
	       void * start,
	       uint32_t size)
{
  int i = 0;
  task_t * task = vma->task;
  // for vma_area's
  void * start_vpf = (void *)((uint32_t)start & PAGE_MASK);
  void * end_vpf = (void *)(((uint32_t)start + size - 1) & PAGE_MASK);
  void * vpf = NULL;
  int num_pages = ((uint32_t)(end_vpf - start_vpf) >> PAGE_SHIFT) + 1;
  uint32_t mapping = 0;

  kdinfo("vm_area_create, vmarealen: %u, pages: %u",
	 size, 
	 num_pages);


  for (i = 0; i < num_pages; i++) {
    vpf = start_vpf + (i << PAGE_SHIFT);

    /* Check if pgtable was allocated, and allocate here */
    
    mapping = vmm_get_mapping(task->page_dir, 
			      vpf);
    kdinfo("Mapping: 0x%x", mapping);

    if ((mapping & PAGE_MASK) == 0) {
      
      /* There's no physical page-frame, allocate it */
      void * ppf = vmm_ppf_alloc();
   
      if (ppf == NULL) {
	return -1;
      }
   
      kdverbose("Allocated page: %p", ppf);
      /* This will also allocate the page table, if that's
       * not there
       */
      vmm_map_page(task->page_dir,
		   vpf,
		   ppf,
		   vma->flags);
      
       kdverbose("vmarea: N task %d, vpf:%p, ppf:%p, flags: 0x%x",
	      task->id,
	      vpf,
	      ppf,
	      vma->flags);
       
    } else { 
      // replace page attributes
      vmm_map_page(task->page_dir,
		   vpf,
		   (void *)(mapping & PAGE_MASK),
		   vma->flags);
      
      kdverbose("vmarea: M task %d, vpf:%p, ppf:%p, flags: 0x%x",
	     task->id,
	     vpf,
	     (void *)(mapping & PAGE_MASK),
	     vma->flags);
      
    }

  }

  vmm_flush_mmu_update_queue();

  return 0;
}

/*
 * Zero out an already allocated part of the VMA
 */
int
vm_area_zero(struct vm_area * vma,
	     void * addr,
	     uint32_t length)
{
  if (vma->start_address > addr ||
      vma->start_address + vma->size < addr + length) {
    // Out of bounds
    return -1;
  }
  void * start_vpf = (void *)((uint32_t)addr & PAGE_MASK);
  void * end_vpf = (void *)(((uint32_t)addr + length - 1) & PAGE_MASK);
  int num_pages = ((uint32_t)(end_vpf - start_vpf) >> PAGE_SHIFT) + 1;
  uint32_t pg_offset = ((uint32_t)addr & PAGE_DIV);
  int total_bytes_zeroed = 0;
  int i = 0;

  for (i = 0; i < num_pages; i++) {
    int bytes_to_zero = length - total_bytes_zeroed;
    void * vpf = start_vpf + (i << PAGE_SHIFT);
    void * ppf = (void *)((uint32_t)vmm_get_mapping(vma->task->page_dir, 
    					    vpf) & PAGE_MASK);
    
    if (ppf != NULL) {

      ppf = VMM_P2V(ppf);
      
      if (pg_offset + bytes_to_zero > PAGE_SIZE) {
	bytes_to_zero = PAGE_SIZE - pg_offset;
      }
    
      memset(ppf + pg_offset, 0, bytes_to_zero);
    
      kdinfo("Vmazero: vpf: %p, ppf; %p, pgoff: 0x%x, zeroed_bytes: 0x%x",
	     vpf,
	     ppf,
	     pg_offset,
	     bytes_to_zero);
    }
    pg_offset = 0;
  }

  vmm_flush_mmu_update_queue();
    
  return 0;
}

int
vm_area_load(struct vm_area * vma,
	     void * load_addr,
	     uint32_t length)
{
  kdverbose("entering vm_area_load");
 
  if (vma->start_address > load_addr ||
      (vma->start_address + vma->size) <
      (load_addr + length) ||
      vma->type != VMA_TYPE_FILE ||
      vma->src.file_src.file == NULL) {
    kderr("Out of bounds or invalid request");
    return -1;
  }
  
  uint32_t foffset = (vma->src.file_src.start_offset + 
		      ((uint32_t)load_addr - 
		       (uint32_t)vma->start_address));

  kdverbose("foffset: %d", foffset);

  int i = 0;
  int bytes_read = 0;
  int total_bytes_read = 0;
  void * start_vpf = (void *)((uint32_t)load_addr & PAGE_MASK);
  void * end_vpf = (void *)(((uint32_t)load_addr + length - 1) & PAGE_MASK);
  
  int num_pages = ((uint32_t)(end_vpf - start_vpf) >> PAGE_SHIFT) + 1;
  uint32_t load_offset = ((uint32_t)load_addr & PAGE_DIV);
  file_t * file = vma->src.file_src.file; 

  for (i = 0; i < num_pages; i++) {
    int bytes_to_read = length - total_bytes_read;
    void * vpf = start_vpf + (i << PAGE_SHIFT);
    void * ppf = (void *)((uint32_t)vmm_get_mapping(vma->task->page_dir, 
						    vpf) & PAGE_MASK);
    ppf = VMM_P2V(ppf);

    if (load_offset + bytes_to_read > PAGE_SIZE) {
      bytes_to_read = PAGE_SIZE - load_offset;
    }
    kdverbose("ppf: %p", ppf);
    /* Zero out the buffer before we read */
    memset(ppf + load_offset, 0, bytes_to_read);
    
    /* Read in the bytes */
    bytes_read = file->getbytes(file,
				foffset + total_bytes_read,
				bytes_to_read,
				ppf + load_offset);
    
    kdinfo("Vmaread: vpf: %p, ppf: %p, foff: 0x%x, pgoff: 0x%x, reqbytes: 0x%x, readbytes: 0x%x",
	   vpf,
	   ppf,
	   foffset + total_bytes_read,
	   load_offset,
	   bytes_to_read,
	   bytes_read);

    if (bytes_read < 0) {
      // read error
      return bytes_read;
    } else if (bytes_read == 0 || 
	       bytes_read != bytes_to_read) {
      // XXX truncated read
      return -1;
    } 

    load_offset = 0;
    total_bytes_read += bytes_read;

    vmm_map_page(vma->task->page_dir,
		 vpf,
		 ppf,
		 vma->flags | PG_FLAG_PRESENT);

  }

  vmm_flush_mmu_update_queue();

  return total_bytes_read;
}

#if 0
static void
vm_area_add_page(vm_area_t * vma,
		 void * pfaddr)
{
  
  return;
}
#endif



/*
 * Page fault handlers
 */
int
vma_nopage_default(vm_area_t * vma,
		   void * vaddr)
{
  panic("Page fault at %p, cannot be handled.",
	vaddr);
  return 0;
}

int
vma_nopage_alloc(vm_area_t * vma,
		 void * vaddr)
{
  void * vpf = (void *)((uint32_t)vaddr & PAGE_MASK);
  /* Allocate an unitialized page frame 
   * for each page fault
   */
  vm_area_create(vma,
		 vpf,
		 PAGE_SIZE);
  return 0;
}

/* Not necesarily a full-page, we have to be careful and 
 * zero out only the area within the memory map
 */
int
vma_nopage_zeropage(vm_area_t * vma,
		    void * vaddr)
{
  uint32_t length = 0;
  void * vpf = (void *)((uint32_t)vaddr & PAGE_MASK);
  
  /* if the vm area does not start on a page boundary, and
   * if this is the first page, then move our read offset 
   * appropriately, so we start zeroing from the first vma byte
   */
  if (vma->start_address >= vpf) {
    vaddr = vma->start_address;
  } else {
    /*
     * This is not the first page of the zeroed vm area
     * So, start zeroing from the top of the page boundary.
     */
    vaddr = vpf;
  }

  uint32_t page_offset = (uint32_t)vaddr - (uint32_t)vpf;
  length = PAGE_SIZE - page_offset;

  void * vma_end_address = vma->start_address + vma->size - 1;
  void * page_end_address = (void *)((uint32_t)vpf | (~PAGE_MASK));

  if (vma_end_address < page_end_address) {
    length -= (uint32_t)page_end_address - (uint32_t)vma_end_address;
  }
  

  /* First allocate a new page */
  if (vm_area_create(vma,
		     vpf,
		     PAGE_SIZE) < 0) {
    return -1;
  }
  
  /* Now zero out the vma */
  vm_area_zero(vma,
	       vaddr,
	       length);
  return 0;

}

int 
vma_pagewrite_write(vm_area_t * vma,
		    void * vaddr)
{
  if (vma->flags & PG_FLAG_WR) {
    /* OK to make region writable
     * 'size' param is set to one so that
     * the page containing this byte
     * is made writable
     */
    if (vm_area_setflags(vma,
			 vaddr,
			 1) < 0) {
      return -1;
    }
    kdinfo("Made page writable");
  } else {
    kdinfo("Attempt to write to ro page %p", vaddr);
    return -1;
  }
  return 0;
}

int
vma_pagewrite_default(vm_area_t * vma,
		      void * vaddr)
{
  kdinfo("Attempt to write to ro page %p", vaddr);
  return -1;
}

/*
 * Called when the target page is present but not
 * writable.
 */
int
vma_pagewrite_cow(vm_area_t * vma,
		  void * vaddr)
{
  void * vpf = (void *)((uint32_t)vaddr & PAGE_MASK);
  /* Now copy the contents from the src vma */
  kdinfo("About to perform copy on write");
  if (vm_area_copy_on_write(vma,
			    vpf,
			    PAGE_SIZE) < 0) {
    return -1;
  }
  
  return 0;
}


int
vma_nopage_file(vm_area_t * vma,
		void * vaddr)
{
  int rc = 0;

  uint32_t length = 0;
  void * vpf = (void *)((uint32_t)vaddr & PAGE_MASK);

  /* if the vm area does not start on a page boundary, and
   * if this is the first page, then move our read offset 
   * appropriately, so we start reading from the first file offset
   */
  if (vma->start_address >= vpf) {
    vaddr = vma->start_address;
  } else {
    /*
     * This is not the first page of the file-backed vm area
     * So, start reading from the top of the page boundary.
     */
    vaddr = vpf;
  }

  uint32_t page_offset = (uint32_t)vaddr - (uint32_t)vpf;
  length = PAGE_SIZE - page_offset;
  void * vma_end_address = vma->start_address + vma->size - 1;
  void * page_end_address = (void *)((uint32_t)vpf | (~PAGE_MASK));
  
  if (vma_end_address < page_end_address) {
    length -= (uint32_t)page_end_address - (uint32_t)vma_end_address;
  }
  
  /* First allocate the page */
  rc = vm_area_create(vma,
		      vpf,
		      PAGE_SIZE);
  if ( rc < 0) {
    return rc;
  }
  
  /* Now load the file data for the 
   * range we have mapped in.
   * The area_load takes care of reading the file offset
   */
  rc = vm_area_load(vma,
		    vaddr,
		    length);

  vmm_flush_mmu_update_queue();

  return  rc;
}

int
vma_nopage_copy(vm_area_t *vma,
		void * vaddr)
{
  
  kdinfo("vma_nopage_copy: Using vmap policy for %p from src vma %p",
	 vaddr,
	 vma->src.vma);

  return vma->src.vma->nopage(vma, vaddr);
}


/*
 * VMA initializers
 */
void
vm_area_init(vm_area_t * vma,
	     vm_area_list_t * vma_list,
	     task_t * task,
	     void * start_address,
	     uint32_t size,
	     uint32_t flags,
	     uint32_t type,
	     uint32_t vma_flags)
{
  memset(vma, 0, sizeof(vm_area_t));
  
  /*
  if (start_address == NULL && 
      (type == VMA_TYPE_ALLOC ||
       type == VMA_TYPE_ZEROPAGE)) {
    start_address = vm_area_find_start_address(vma_list);
  } 
  */

  vma->task = task;
  task_get(task);
  vma->start_address = start_address;
  vma->size = size;
  vma->flags = flags;
  vma->type = type;
  vma->vma_flags = vma_flags;
  vma->nopage = vma_nopage_default;

  if ((vma->flags & PG_FLAG_WR) &&
      (vma->flags & PG_FLAG_USR)) {
    vma->pagewrite = vma_pagewrite_write;
  } else {
    vma->pagewrite = vma_pagewrite_default;
  }

  kdinfo("Init VMA:%p, tsk:%p:%d, start: 0x%x, size:0x%x, flg:0x%x, type:%s, vmf:0x%x",
	 vma,
	 vma->task,
	 vma->task->id,
	 vma->start_address,
	 vma->size,
	 vma->flags,
	 vma_get_type(vma->type),
	 vma->vma_flags);
}

int
vm_area_init_kernel_stack(vm_area_t * vma)
{
  unsigned int num_pages = ((vma->size - 1) >> PAGE_SHIFT) + 1;
  void * kernel_stack = NULL;
  
  vma->nopage = vma_nopage_default;
  
  kdinfo("Allocating %u bytes for kernel stack (%u pages)", 
	  vma->size,
	  num_pages);

  // Allocate stack from high memory
  kernel_stack = vmm_ppfs_highmem_alloc(num_pages);
 
  if (kernel_stack == NULL) {
    kdinfo("Unable to allocate kernel stack");
    return -1;
  }

  vma->start_address = kernel_stack;
  vma->src.paddr = vma->start_address;

  /*
   * Also create the identity mapping here.
   */ 
  if (vm_area_create_identity(vma) < 0) {
    kdinfo("Unable to create kernel stack mappings");
    vm_area_unmap(vma);
    vmm_ppfs_highmem_free(kernel_stack, 
			  num_pages);
    return -1;
  }

  return 0;

}

int
vm_area_init_direct(vm_area_t * vma,
		    vm_area_src_t * src)
{
  vma->src.paddr = src->paddr;
  //vma->start_address = vma->src.paddr;
  vma->nopage = vma_nopage_default;
  
  kdinfo("vma dir: phys:%p, saddr:%p",
	 vma->src.paddr,
	 vma->start_address);

  /*
   * Also create the identity mapping here.
   */ 
  return vm_area_create_identity(vma);

}

int
vm_area_init_alloc(vm_area_t * vma)
{  
  vma->nopage = vma_nopage_alloc;
  if (vm_area_create_null(vma) < 0) {
    kdinfo("Unable to init Alloc VMA");
    vm_area_unmap(vma);
    return -1;
  }
  return 0;
}

int
vm_area_init_zeropage(vm_area_t * vma)
{
  vma->nopage = vma_nopage_zeropage;
  if (vm_area_create_null(vma) < 0) {
    kdinfo("Unable to init zpage VMA");
    vm_area_unmap(vma);
    return -1;
  }
  return 0;
}

int
vm_area_init_copy(vm_area_t * vma,
		  vm_area_src_t * src)
{

  int rc = 0;
  vma->start_address = src->vma->start_address;
  vma->size = src->vma->size;
  vma->flags = src->vma->flags;
  vma->type = src->vma->type;
  vma->nopage = src->vma->nopage;
  vma->pagewrite = src->vma->pagewrite;

  /* Make sure we don't go off calling a chain of 'nopage's */
  if (src->vma->type == VMA_TYPE_COPY ||
      src->vma->type == VMA_TYPE_COW) {
    vma->src.vma = src->vma->src.vma;
  } else {
    vma->src.vma = src->vma;
  }

  rc =  vm_area_copy(vma,
		     vma->start_address,
		     vma->size);
  
  if (rc < 0) {
    /*Allocation failed for some elements in
     * the page map, abort creation
     */
    kdinfo("Unable to init Copy VMA");
    vm_area_unmap(vma);
    return -1;
  }

  if (vma->type == VMA_TYPE_FILE) {
    vma->src.file_src.file = src->vma->src.file_src.file;
    file_get(vma->src.file_src.file);
  }
  
  return 0;
}

int
vm_area_init_cow(vm_area_t * vma,
		 vm_area_src_t * src)
{
  vma->src.vma = src->vma;
  vma->nopage = vma_nopage_copy;
  vma->pagewrite = vma_pagewrite_cow;
  vma->start_address = src->vma->start_address;

  /* Make sure we don't go off calling a chain of 'nopage's */
  if (src->vma->type == VMA_TYPE_COPY ||
      src->vma->type == VMA_TYPE_COW) {
    vma->src.vma = src->vma->src.vma;
  } else {
    vma->src.vma = src->vma;
  }

  if (vm_area_copy_pagemap(vma,
			   PG_FLAG_USR,
			   ~PG_FLAG_WR) < 0) {
    kdinfo("Unable to init COW VMA");
    vm_area_unmap(vma);
    return -1;
  }
  
  /* Ready to own resources */
  vm_area_get(src->vma);

  return 0;
}

int
vm_area_init_file(vm_area_t * vma,
		  vm_area_src_t * src)
{
  vma->src.file_src = src->file_src;
  
  vma->nopage = vma_nopage_file;
  
  if (vm_area_create_null(vma) < 0) {
    kdinfo("Unable to init file VMA");
    vm_area_unmap(vma);
    return -1;
  }

  /* Ready to own resources */
  file_get(src->file_src.file);

  return 0;
  
}

vm_area_t *
vm_area_alloc(vm_area_list_t * vma_list,
	      task_t * task,
	      void * start_address,
	      uint32_t size,
	      uint32_t flags,
	      uint32_t type,
	      uint32_t vma_flags,
	      vm_area_src_t * src)
{

  vm_area_t * vma = (vm_area_t *)malloc(sizeof(vm_area_t));
  int rc = 0;

  if (vma == NULL) {
    kdinfo("Unable to allocate VMA struct");
    return NULL;
  }

  vm_area_init(vma,
	       vma_list,
	       task,
	       start_address,
	       size,
	       flags,
	       type,
	       vma_flags);
  

  /* Now initialize the rest of the state */
  if (type == VMA_TYPE_COPY) {
    /* Set up the initial mapping */
    /* Ignores flags etc fields, copying everything
     * from the src
     */
    rc = vm_area_init_copy(vma, src);
  } else if (type == VMA_TYPE_COW) {
    rc = vm_area_init_cow(vma, src);

    /* If we are creating the master cow vma
     * add it to the vma list, replacing the
     * source copy
     */
    if (rc == 0 && 
	vma->task == src->vma->task) {
      src->vma->flags = PGTAB_ATTRIB_USR_RO;
      vm_area_replace(&vma->task->vm_area_list,
		      src->vma, 
		      vma);
      return vma; 
    }
  } else if (type == VMA_TYPE_FILE) {
    rc = vm_area_init_file(vma, src);
  } else if (type == VMA_TYPE_ZEROPAGE) {
    rc = vm_area_init_zeropage(vma);
  } else if (type == VMA_TYPE_ALLOC) {
    rc = vm_area_init_alloc(vma);
  } else if (type == VMA_TYPE_DIRECT) {
    rc = vm_area_init_direct(vma, src);
  } else if (type == VMA_TYPE_KERNEL_STACK) {
    rc = vm_area_init_kernel_stack(vma);
  } else {
    panic("Invalid type 0x%x while creating VMA", type);
  }
  
  if (rc < 0) {
    kdinfo("Error creating VMA");
    free(vma);
    return NULL;
  }

  kdinfo("VMA ALLOC: vma:%p, start:%p, size:0x%x, type:%s, flags: 0x%x, cnt:%d",
	 vma, 
	 vma->start_address, 
	 vma->size,
	 vma_get_type(vma->type),
	 vma->flags,
	 vma->count);


  vm_area_add(vma_list, vma);

  return vma;
}

void
vm_area_free_resources(vm_area_t * vma)
{

  /* if this is a file type VMA, put the file */
  if (vma->type == VMA_TYPE_FILE) {
    kdinfo("Releasing file handle %p for vma %p",
	   vma->src.file_src.file,
	   vma);
    file_put(vma->src.file_src.file);
    vma->src.file_src.file = NULL;
    vma->src.file_src.start_offset = 0;
  }
  kdinfo("Unmapping vma %p", vma);
  /* Unmap and free all pages belonging to this VMA */
  vm_area_unmap(vma);
  /*
   * Kernel stack pages are deallocated specially, since
   * they are allocated as a block
   */
  if (vma->type == VMA_TYPE_KERNEL_STACK) {
    unsigned int num_pages = ((vma->size - 1) >> PAGE_SHIFT) + 1;
    vmm_ppfs_free(vma->start_address,
		  num_pages);
  }

}

void
vm_area_free(vm_area_t * vma)
{
  kdinfo("Releasing vma resources for %p t:%s, tsk: %p:%d", 
	 vma,
	 vma_get_type(vma->type),
	 vma->task,
	 vma->task->id);

  if (vma->type != VMA_TYPE_DIRECT) {
    vm_area_free_resources(vma);
  }

  task_put(vma->task);
  kdinfo("Freeing vma %p", vma);
  free(vma);
}

void
vm_area_get(vm_area_t * vma)
{
  vma->count++;
  kdinfo("REFCNT INC VMA %p:%d", vma, vma->count);
}

void
vm_area_put(vm_area_t * vma)
{
  vma->count--;
  kdinfo("REFCNT DEC VMA %p:%d", vma, vma->count);
  assert(vma->count >= 0);
  if (vma->count == 0) {
    vm_area_free(vma);
  }
}


void
vm_area_add(vm_area_list_t * vma_list,
	    vm_area_t * vma)
{
  vm_area_t * curpos = NULL;
  kdverbose("Incrementing VMA refcnt");
  vm_area_get(vma);
  Q_FOREACH(curpos, vma_list, task_link) {
    if (curpos->start_address > vma->start_address) {
      kdinfo("Inserting VMA");
      Q_INSERT_BEFORE(vma_list, curpos, vma, task_link);
      return;
    }
  }
  kdinfo("Appending VMA to %p", vma_list);
  /* Make this a kdtrace to step per VMA allocation */
  kdinfo("vma_list: f:%p, l:%p", vma_list->first, vma_list->last);
  Q_INSERT_TAIL(vma_list, vma, task_link);

}

/* Seemed useful when I wrote it, currently unused */
void
vm_area_replace(vm_area_list_t * vma_list,
		vm_area_t * inq_vma,
		vm_area_t * new_vma)
{
  Q_INSERT_BEFORE(vma_list, 
		  inq_vma, 
		  new_vma, 
		  task_link);
  
  Q_REMOVE(vma_list, inq_vma, task_link);
}

void
vm_area_remove_taskvm(vm_area_list_t * vma_list)
{
  vm_area_t * curpos = NULL;
  vm_area_t * nextpos = NULL;
  kdinfo("Removing All VMAs");
  for (curpos = vma_list->first;
       curpos != NULL;
       curpos = nextpos) {
    nextpos = curpos->task_link.next;
    if (curpos->type != VMA_TYPE_KERNEL_STACK &&
	curpos->type != VMA_TYPE_DIRECT &&
	(curpos->vma_flags & VMA_FLAG_STACK) == 0) { 
      vm_area_remove(vma_list,
		     curpos);
    }
  }

}

void
vm_area_remove_all(vm_area_list_t * vma_list)
{
  vm_area_t * curpos = NULL;
  vm_area_t * nextpos = NULL;
  kdinfo("Removing All VMAs");
  for (curpos = vma_list->first;
       curpos != NULL;
       curpos = nextpos) {
    nextpos = curpos->task_link.next;
    vm_area_remove(vma_list,
		   curpos);
  }
}

void
vm_area_remove(vm_area_list_t * vma_list,
	       vm_area_t * vma) 
{
  Q_REMOVE(vma_list, vma, task_link);
  vm_area_put(vma);
}

vm_area_t *
find_vm_area_overlap(vm_area_list_t * vma_list,
		     void * vaddr,
		     int size)
{
  vm_area_t * curpos = NULL;
  Q_FOREACH(curpos, vma_list, task_link) {

    kdinfo("findvmaovlp: vma:%p, start:%p, size:0x%x, type:%s, flags: 0x%x, cnt:%d, vaddr:0x%x, vsize:0x%x",
	   curpos, 
	   curpos->start_address, 
	   curpos->size,
	   vma_get_type(curpos->type),
	   curpos->flags,
	   curpos->count,
	   vaddr,
	   size);
    
    void * start_addr = (void *)((uint32_t)curpos->start_address);
    
    void * end_addr = (void *)((uint32_t)curpos->start_address + 
			       curpos->size - 1);
    
    if ((vaddr == start_addr) || 
	(vaddr == end_addr)) {
      return curpos;
    }

    if ((vaddr < start_addr) && 
	((vaddr + size - 1) >= start_addr)) {
      return curpos;
    }

    if ((vaddr > start_addr) &&
	(vaddr < end_addr)) {
      return curpos;
    }

  }

  return NULL;
}


vm_area_t *
find_vm_area(vm_area_list_t * vma_list,
	     void * vaddr)
{
  vm_area_t * curpos = NULL;
  Q_FOREACH(curpos, vma_list, task_link) {

    kdinfo("findvma: vma:%p, start:%p, size:0x%x, type:%s, flags: 0x%x, cnt:%d, vaddr:0x%x",
	   curpos, 
	   curpos->start_address, 
	   curpos->size,
	   vma_get_type(curpos->type),
	   curpos->flags,
	   curpos->count,
	   vaddr);
    
    uint32_t start_vpf = ((uint32_t)curpos->start_address & PAGE_MASK);
    uint32_t end_vpf = ((uint32_t)curpos->start_address + 
			curpos->size - 1) & PAGE_MASK;
    uint32_t vpf = (uint32_t)vaddr & PAGE_MASK;
    
    if (vpf >= start_vpf && vpf <= end_vpf){
      return curpos;
    }

  }
  return curpos;
}
