/**
 * The 15-410 kernel project.
 * @name loader.c
 *
 * Functions for the loading
 * of user programs from binary 
 * files should be written in
 * this file. The function 
 * elf_load_helper() is provided
 * for your use.
 */
/*@{*/

/* --- Includes --- */
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <malloc.h>
#include <exec2obj.h>
#include <loader.h>
#include <elf_410.h>


/* --- Local function prototypes --- */ 


/**
 * Copies data from a file into a buffer.
 *
 * @param filename   the name of the file to copy data from
 * @param offset     the location in the file to begin copying from
 * @param size       the number of bytes to be copied
 * @param buf        the buffer to copy the data into
 *
 * @return returns the number of bytes copied on succes; -1 on failure
 */
int getbytes( const char *filename, int offset, int size, char *buf )
{
  int i = 0;
  int found = 0;
  int bytes_to_read = 0;
  exec2obj_userapp_TOC_entry const * toc_entry = NULL;
  for (i = 0; i < exec2obj_userapp_count; i++) {
      toc_entry = &exec2obj_userapp_TOC[i];
    if (!strcmp(filename, toc_entry->execname)) {
      found  = 1;
      break;
    }
  }

  if (!found) {
    return -1;
  }
  
  if (offset >= toc_entry->execlen) {
    return 0;
  }

  bytes_to_read = size;

  if (bytes_to_read + offset > toc_entry->execlen) {
    bytes_to_read = toc_entry->execlen - offset;
  }
  
  memcpy(buf, 
	 toc_entry->execbytes + offset, 
	 bytes_to_read);

  return bytes_to_read;
}

/*@}*/
