/** @file rwlock.h
 *  @brief This file defines the interface to readers/writers locks
 */

#ifndef RWLOCK_H
#define RWLOCK_H

#define RWLOCK_READ  0
#define RWLOCK_WRITE 1

#include <rwlock_type.h>

/* readers/writers lock functions */
int rwlock_init( rwlock_t *rwlock );
int rwlock_lock( rwlock_t *rwlock, int type );
int rwlock_unlock( rwlock_t *rwlock );
int rwlock_destroy( rwlock_t *rwlock );

#endif /* RW_H */
