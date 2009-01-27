#ifndef __FILE_H__
#define __FILE_H__
#include <stdint.h>

#define MAX_FILENAME_LEN 1024

typedef struct file {
  char name[MAX_FILENAME_LEN];
  uint32_t offset;
  
  int (*getbytes)(struct file * file,
		  uint32_t offset,
		  uint32_t len,
		  char * buf);
  int count;
} file_t;

file_t *
file_alloc(void);

void
file_get(file_t * file);

void
file_put(file_t * file);

int
file_open(file_t * file,
	  char * filename);


#endif
