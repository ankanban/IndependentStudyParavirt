/** @file mutex.h
 *  @brief This file defines the interface for mutexes.
 */

#ifndef MUTEX_H
#define MUTEX_H

#include <mutex_type.h>

int mutex_init( mutex_t *mp );
int mutex_destroy( mutex_t *mp );
int mutex_lock( mutex_t *mp );
int mutex_unlock( mutex_t *mp );

#endif /* MUTEX_H */
