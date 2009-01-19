#define DEBUG_LEVEL KDBG_INFO
#include <stddef.h>
#include <stdlib.h>
#include <malloc.h>
#include <malloc_internal.h>
#include <vmm.h>
#include <lmm.h>
#include <kdebug.h>

extern lmm_t malloc_lmm;

/* safe versions of malloc functions */
void *malloc(size_t size)
{

  kdtrace("Allocating %d bytes", size); 
  return _malloc(size);
  /*
  void * node = lmm_alloc(&malloc_lmm, size + sizeof(size_t), 0);

  *((uint32_t *)node) = size + sizeof(size_t);

  node += sizeof(size_t);

  kdtrace("Allocated %d bytes at %p", size + sizeof(size_t), node);
  */
  //return node;

}

void *memalign(size_t alignment, size_t size)
{
  return _memalign(alignment, size);
}

void *calloc(size_t nelt, size_t eltsize)
{
  return _calloc(nelt, eltsize);
}

void *realloc(void *buf, size_t new_size)
{
  return _realloc(buf, new_size);
}

void free(void *buf)
{
  _free(buf);
  return;
  /*
  void * node = (void *)((uint32_t)buf - sizeof(size_t));

  size_t size = *((uint32_t *)node);
  
  kdtrace("Freeing %d bytes at %p", size, node);

  lmm_free(&malloc_lmm, node, size);
  */
}

void *smalloc(size_t size)
{
  return _smalloc(size);
}

void *smemalign(size_t alignment, size_t size)
{
  return _smemalign(alignment, size);
}

void *scalloc(size_t size)
{
  void * addr = smalloc(size); 

  if (addr != NULL) {
    memset(addr, 0, size);
  }
  return addr;
}

void sfree(void *buf, size_t size)
{
  _free(buf);
}


