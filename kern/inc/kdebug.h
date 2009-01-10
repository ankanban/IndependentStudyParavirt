#ifndef __KDEBUG_H__
#define __KDEBUG_H__

#include <simics.h>

#define KDBG_NONE   0
#define KDBG_LOG    1
#define KDBG_ERR    2
#define KDBG_WARN   3
#define KDBG_INFO   4
#define KDBG_TRACE  5
#define KDBG_VERBOSE 6

/* Define DEBUG_LEVEL appropriately
 * in the C file that includes kdebug.h
 */
//#define DEBUG_LEVEL KDBG_TRACE

#ifdef DEBUG_LEVEL

/* Comment these 4 lines to enable 
 * debug logging in all files which
 * use it (unless explcitly forced
 * by setting FORCE_DEBUG)
 */
#ifndef FORCE_DEBUG
#undef DEBUG_LEVEL
#define DEBUG_LEVEL KDBG_LOG
#endif


#define kdprintf(...) lprintf(__VA_ARGS__)

#if (DEBUG_LEVEL >= KDBG_LOG)
#define kdlog(...) kdprintf(__VA_ARGS__)
#else
#define kdlog(...)
#endif

#if (DEBUG_LEVEL >= KDBG_INFO)
#define kdinfo(...) kdprintf(__VA_ARGS__)
#else
#define kdinfo(...)
#endif

#if (DEBUG_LEVEL >= KDBG_WARN)
#define kdwarn(...) kdprintf(__VA_ARGS__)
#else
#define kdwarn(...)
#endif

#if (DEBUG_LEVEL >= KDBG_ERR)
#define kderr(...) kdprintf(__VA_ARGS__)
#else
#define kderr(...)
#endif

#if (DEBUG_LEVEL >= KDBG_VERBOSE)
#define kdverbose(...) kdprintf(__VA_ARGS__)
#else
#define kdverbose(...)
#endif

#if (DEBUG_LEVEL >= KDBG_TRACE)
#define kdtrace(...)				\
  do {						\
    kdprintf(__VA_ARGS__);			\
    MAGIC_BREAK;				\
  }  while(0)
#else
#define kdtrace(...) kdinfo(__VA_ARGS__)
#endif

#else /* DEBUG_LEVEL */

#define kdprintf(...)
#define kdlog(...)
#define kdinfo(...)
#define kdwarn(...)
#define kderr(...)
#define kdtrace(...)
#define kdverbose(...)

#endif /* DEBUG_LEVEL */

#endif /* __KDEBUG_H__ */
