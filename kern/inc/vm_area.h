/** @file vm_area.h 
 *
 *  @brief Virtual Memory region manipulation
 *
 *  @author Anshuman P.Kanetkar (apk)
 * 
 */
#ifndef __VM_AREA_H__
#define __VM_AREA_H__

#include <stdint.h>
#include <stddef.h>
#include <simics.h>
#include <x86/page.h>
#include <vmm_page.h>
#include <variable_queue.h>
#include <file.h>

struct task;

typedef enum {
  VMA_TYPE_KERNEL_STACK,
  VMA_TYPE_DIRECT,
  VMA_TYPE_ALLOC,
  VMA_TYPE_ZEROPAGE,
  VMA_TYPE_COW,
  VMA_TYPE_COPY,
  VMA_TYPE_FILE,
} vma_type_t;

#define VMA_PTE_ATTRIB 0x101
#define VMA_FLAG_SHARED 0x1
#define VMA_FLAG_STACK 0x2

struct vm_area;

typedef struct vm_area_file_src {
  file_t * file;
  uint32_t start_offset;
} vm_area_file_src_t ;


struct vm_area;

typedef union {
  void * paddr;
  struct vm_area * vma;
  vm_area_file_src_t file_src;
} vm_area_src_t;

typedef struct vm_area {
  void * start_address;

  uint32_t size;
  uint32_t flags;
  uint32_t type;
  int count;
  uint32_t vma_flags;

  struct task * task;
  
  vm_area_src_t src;
  
  /* VMA methods */
  int 
  (*nopage)(struct vm_area * vma,
	    void * vaddr);

  int
  (*pagewrite)(struct vm_area * VMA,
	       void * vaddr);

  /* List pointers */
  Q_NEW_LINK(vm_area) task_link;
} vm_area_t;

Q_NEW_HEAD(vm_area_list_t, vm_area);

/*
 * given a virtual address, find the
 * VM area responsible for it.
 */
vm_area_t *
find_vm_area(vm_area_list_t * vma_list,
	     void * vaddr);

vm_area_t *
find_vm_area_overlap(vm_area_list_t * vma_list,
		     void * vaddr,
		     int size);

vm_area_t *
vm_area_alloc(vm_area_list_t * vma_list,
	      struct task * task,
	      void * start_address,
	      uint32_t size,
	      uint32_t flags,
	      uint32_t type,
	      uint32_t vma_flags,
	      vm_area_src_t * src);
void
vm_area_free(vm_area_t * vma);

void
vm_area_add(vm_area_list_t * vma_list,
	    vm_area_t * vma);

void
vm_area_remove(vm_area_list_t * vma_list,
	       vm_area_t * vma);

int
vm_area_zero(struct vm_area * vma,
	     void * addr,
	     uint32_t length);

void
vm_area_replace(vm_area_list_t * vma_list,
		vm_area_t * inq_vma,
		vm_area_t * new_vma);

void
vm_area_remove_all(vm_area_list_t * vma_list);
		    
void
vm_area_remove_taskvm(vm_area_list_t * vma_list);

void
vm_area_get(vm_area_t * vma);

void
vm_area_put(vm_area_t * vma);


#endif /* __VM_AREA_H__ */
