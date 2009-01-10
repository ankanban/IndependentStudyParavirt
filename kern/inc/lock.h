#ifndef __LOCK_H__
#define __LOCK_H__

#include <stdint.h>

typedef uint32_t mutex_t;

#define MUTEX_LOCKED 0
#define MUTEX_UNLOCKED 1

int 
mutex_spinlock(mutex_t * mutex);

int
mutex_trylock(mutex_t * mutex);

void
mutex_unlock(mutex_t * mutex);

#endif /* __LOCK_H__ */
