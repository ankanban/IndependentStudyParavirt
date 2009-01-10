#ifdef BUDDY_TEST
#include "variable_queue.h"
#include <stdio.h>
#include<string.h>
#include <stdlib.h>
#else
#define DEBUG_LEVEL KDBG_LOG
#include <variable_queue.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <vmm_zone.h>
#include <kdebug.h>
#include <x86/page.h>
#include <kernel.h>
#include <vmm_page.h>
#endif



vmb_free_area_t * 
vmm_buddy_init(void * mem_start,
	       int num_pages,
	       int * max_order)
{
  kdinfo("free mem start: %p", mem_start);
  vmb_free_area_t * free_area_list = (vmb_free_area_t *)mem_start;

  int upper_order = vmm_floor_log2(num_pages);
  int free_area_struct_pages = ((sizeof(vmb_free_area_t) * upper_order - 1) >> 
				PAGE_SHIFT) + 1;
  kdinfo("space for free_area_struct: %d pages", free_area_struct_pages);
  void * first_page = (void *)((uint32_t)mem_start + 
			       (free_area_struct_pages << PAGE_SHIFT));
  kdinfo("First page: %p", first_page);
  num_pages -= free_area_struct_pages;
  upper_order = vmm_floor_log2(num_pages);


  kdinfo("buddy: l:2 ^ 0x%x, u:2 ^ 0x%x",
	 0,
	 upper_order);
  int i = 0;
  for (i = 0; i <= upper_order; i++) {
    memset(&free_area_list[i], 
	   0, sizeof(vmb_free_area_t));
  }
  vmb_free_node_t * node = (vmb_free_node_t *)first_page;
  node->order = upper_order;
  node->buddy = NULL;
  free_area_list[upper_order].num_free++;
  Q_INSERT_FRONT(&free_area_list[upper_order].free_list, 
		 node, 
		 free_link);
  *max_order = upper_order;
  return free_area_list;
}

void
buddy_print_node(vmb_free_node_t *node)
{
  kdinfo("Node:%p, order:%d", node, node->order);
}

void *
buddy_split_node(vmb_free_area_t * free_area_list,
		 vmb_free_node_t * node, 
		 int order)
{
  int i = 0;
  for (i = node->order; i > order; i--) {

    vmb_free_node_t * new_node = NULL;
    
    node->order--;

    /* Need to split */
    new_node = (vmb_free_node_t *)((uint32_t)node + 
			       (PAGE_SIZE << node->order));
    
    new_node->order = node->order;
    new_node->buddy = node;
    node->buddy = new_node;

    free_area_list[node->order].num_free++;
    Q_INSERT_FRONT(&free_area_list[node->order].free_list, 
		   new_node,
		   free_link);

    kdinfo("Split node:");
    buddy_print_node(new_node);

    kdinfo("Splitting node:");
    buddy_print_node(node);

  } 

  if (node->order == order) {
    return (void *)node;
  }
  
  return NULL;
}

void *
vmm_buddy_get_free_pages(vmm_zone_t * zone,
			 int order)
{
  int i = 0;
  vmb_free_area_t * free_area_list = zone->free_area_list;
  int upper_order = zone->max_order;
  for (i = order; i <= upper_order; i++) {
    if (free_area_list[i].num_free == 0) {
      continue;
    }

    kdinfo("Smallest free order: %d, num_free:%d",
	   i,
	   free_area_list[i].num_free);

    /* Found the smallest free area that contains the pages we need */
    free_area_list[i].num_free--;
    vmb_free_node_t * node = Q_GET_FRONT(&free_area_list[i].free_list);  
    Q_REMOVE(&free_area_list[i].free_list, node, free_link);
    return buddy_split_node(free_area_list, node, order);
    
  }
  return NULL;
}

void
vmm_buddy_free_pages(vmm_zone_t * zone,
		     void * pgaddr, 
		     int order)
{
  int i = 0;
  vmb_free_area_t * free_area_list = zone->free_area_list;
  vmb_free_node_t * node = (vmb_free_node_t *)pgaddr;
  node->order = order;
  int upper_order = zone->max_order;
  for (i = order; i <= upper_order; i++) {
        /* Scan the free list for the buddy */
    vmb_free_node_t * curpos;
    vmb_free_node_t * buddy = (vmb_free_node_t *)((uint32_t)node + 
					  (PAGE_SIZE << i));
    
    Q_FOREACH(curpos, &free_area_list[i].free_list, free_link) {
      kdinfo("Trying to merge with node %p, order: %d",
	     curpos, curpos->order);
      if (curpos > node) {
	if (buddy != curpos) {
	  continue;
	}
      } else if (curpos < node) {
	vmb_free_node_t * lo_buddy = (vmb_free_node_t *)((uint32_t)curpos + 
						 (PAGE_SIZE << i));
	if (lo_buddy == node) {
	  node = curpos;
	} else {
	  continue;
	}	
      } else if (curpos == node) {
	/* Double free XXX Panic */
	kdtrace("Attempt to double free a region %p", node);
	return;
      }
      kdinfo("Found buddy: %p, order: %d", curpos, curpos->order);
      node->order++;
      free_area_list[i].num_free--;
      Q_REMOVE(&free_area_list[i].free_list, curpos, free_link);
      break;
    }
    
    if (curpos == NULL) {
      /* Nothing to merge */
      break;
    }
    /* Try to merge with a higher order node */
  }

  kdinfo("Adding merged node: %p, order:%d", node, node->order);
  /* Insert the newly merged node in its proper place */
  free_area_list[node->order].num_free++;
  Q_INSERT_FRONT(&free_area_list[node->order].free_list,
		 node,
		 free_link);
}

void
vmm_zone_init(vmm_zone_t * zone,
	      void * start,
	      int num_pages)
{
  zone->free_area_list = 
    vmm_buddy_init(start, 
		   num_pages,
		   &zone->max_order);
  zone->start = start;
  zone->num_pages = num_pages;
  kdinfo("Zone: %p, start: %p, num_pages: %d",
	 zone, start, num_pages);
}

#ifdef BUDDY_TEST
vmm_zone_t lomem_zone;
vmm_zone_t himem_zone;


int
main(int argc, char ** argv)
{
  int num_pages = 0xfff0;
  int highmem_pages = 1024;
  int lomem_pages = num_pages - highmem_pages; 
  void * lomem_free_area = malloc(num_pages << PAGE_SHIFT );
  void * highmem_free_area = (void *)((uint32_t)lomem_free_area + 
				      (lomem_pages << PAGE_SHIFT));
  kdinfo("Lomem: %p, pages: %d", lomem_free_area, lomem_pages);
  vmm_zone_init(&lomem_zone, lomem_free_area, lomem_pages);
  kdinfo("Himem: %p, pages: %d", highmem_free_area, highmem_pages);
  vmm_zone_init(&himem_zone, highmem_free_area, highmem_pages);

  vmm_zone_t * z1 = &lomem_zone;
  vmm_zone_t * z2 = &himem_zone;

  void * p1 = vmm_buddy_get_free_pages(z1,1);
  void * p2 = vmm_buddy_get_free_pages(z1, 2);
  void * p3 = vmm_buddy_get_free_pages(z1, 3);
  void * p12 = vmm_buddy_get_free_pages(z1, 12);

  vmm_buddy_free_pages(z1, p12, 12);
  vmm_buddy_free_pages(z1, p1, 1);
  vmm_buddy_free_pages(z1, p3, 3);
  vmm_buddy_free_pages(z1, p2, 2);


  void * hp1 = vmm_buddy_get_free_pages(z2,2);
  void * hp2 = vmm_buddy_get_free_pages(z2,2);
  vmm_buddy_free_pages(z2, hp1, 2);
  vmm_buddy_free_pages(z2, hp2, 2);

  

  free(lomem_free_area);
}

#endif
