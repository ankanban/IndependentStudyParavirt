/** @file process.h
 *
 *  @brief Task/Thread manipulation routines
 *
 *  @author Anshuman P.Kanetkar (apk)
 * 
 */
#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <task.h>

int
sys_gettid(void);

int 
sys_fork(void);

int
do_exec(task_t * task,
	thread_t * thread,
	char * path,
	char ** args);
int 
do_exec_noyield(task_t * task,
		char * path,
		char * argvec[]);


int
sys_exec(void * sysarg);


#endif /* __PROCESS_H__ */
