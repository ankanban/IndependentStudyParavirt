/** @file timer.h 
 *
 *  @brief Timer Driver implementation definitions.
 *
 *  @author Anshuman P.Kanetkar (apk)
 * 
 */
#ifndef __TIMER_H__
#define __TIMER_H__

//#include <p1kern.h>
#include <simics.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <x86/asm.h>
#include <x86/seg.h>
#include <x86/interrupt_defines.h>
#include <x86/timer_defines.h>
#include <x86/keyhelp.h>
#include <x86/video_defines.h>
#include <inc/interrupt_wrappers.h>
#include <kernel.h>

// Timer macros
#define PC_TIMER_HZ 1193182
#define TIMER_INTERRUPT_HZ 100
#define TIMER_CYCLES_PER_INT (PC_TIMER_HZ/TIMER_INTERRUPT_HZ)

void
timer_init(void (*tickback_upper)(unsigned int),
	   void (*tickback_lower)(void));

#endif /* __TIMER_H__ */
