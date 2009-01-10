#define DEBUG_LEVEL KDBG_INFO
#include <file.h>
#include <stdlib.h>
#include <elf_410.h>
#include <string.h>
#include <kdebug.h>

static int
static_getbytes(file_t * file,
		uint32_t offset,
		uint32_t len,
		char * buf)
{
  return getbytes(file->name,
		  offset,
		  len,
		  buf);
		  
}

file_t *
file_alloc()
{
  file_t * file = (file_t *)malloc(sizeof(file_t));
  memset(file, 0, sizeof(file_t));
  return file;
}


void
file_get(file_t * file)
{
  file->count++;
  kdinfo("REFCNT INC FILE %p:%d", file, file->count);
}


void
file_put(file_t * file)
{
  file->count--;
  kdinfo("REFCNT DEC FILE %p:%d", file, file->count);
  if (file->count == 0) {
    free(file);
  }
}

int
file_open(file_t * file,
	  char * filename)
{
  file->offset = 0;
  file->getbytes = static_getbytes;
  strncpy(file->name, filename, MAX_FILENAME_LEN);
  /* Does nothing */
  return 0;
}

