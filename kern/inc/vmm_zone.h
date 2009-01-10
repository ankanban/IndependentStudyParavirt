/** @file vmm_zone.h 
 *
 *  @brief Buddy allocator implementation for page allocation.
 *
 *  @author Anshuman P.Kanetkar (apk)
 * 
 */
#ifndef __VMM_ZONE_H__
#define __VMM_ZONE_H__

#ifdef BUDDY_TEST

#include "variable_queue.h"
#include <stdio.h>
#include<string.h>
#include <stdlib.h>

typedef unsigned int uint32_t;
#define PAGE_SHIFT 12
#define PAGE_SIZE 4096
#define kdinfo(...)				\
  do {						\
    printf(__VA_ARGS__);			\
    printf("\n");				\
  } while (0)
#define kdtrace(...) kdinfo(__VA_ARGS__)
#else

#include <variable_queue.h>

#endif

/**
 *@brief Free node in the free area table.
 *
 */
typedef struct vmb_free_node {
  unsigned int order;
  void * buddy;
  Q_NEW_LINK(vmb_free_node) free_link;
} vmb_free_node_t;

Q_NEW_HEAD(vmb_free_node_list_t, vmb_free_node);

/**
 *  @brief Free area, used per 'order' of page collections
 */
typedef struct vmb_free_area {
  int num_free;
  vmb_free_node_list_t free_list;
} __attribute__((packed)) vmb_free_area_t;

/**
 * @brief A zone of usable pages. contains a free-list of pages
 * to allocate from the region
 */
typedef struct vmm_zone {
  /* Array of free area structs */
  vmb_free_area_t * free_area_list;
  void * start;
  int num_pages;
  int max_order;
} vmm_zone_t;

/**
 * @brief Allocate 2^order pages.  
 */
void *
vmm_buddy_get_free_pages(vmm_zone_t * zone,
			 int order);

/**
 * @brief Release pages to the free pool.
 */
void
vmm_buddy_free_pages(vmm_zone_t * zone,
		     void * pgaddr, 
		     int order);

/**
 * @brief Initialize a memory zone
 * @param zone Pointer to the struct to initialize
 * @param start Start address for the zone
 * @param num_pages Number of pages to add to the zone.
 */
void
vmm_zone_init(vmm_zone_t * zone,
	      void * start,
	      int num_pages);

#endif /* __VMM_ZONE_H__ */
