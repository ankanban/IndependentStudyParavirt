/** @file keyboard.h 
 *
 *  @brief Keyboard Driver implementation definitions.
 *
 *  @author Anshuman P.Kanetkar (apk)
 * 
 */
#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

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

// Keyboard scancode macros
#define SCANCODE_BUFFER_SIZE_BITS 10

#define SCANCODE_BUFFER_SIZE (1 << SCANCODE_BUFFER_SIZE_BITS)

#define SCANCODE_BUFFER_SIZE_MASK (SCANCODE_BUFFER_SIZE - 1)

#define SCANCODE_BUFFER_NEXTLOC(x) (((x) + 1) & SCANCODE_BUFFER_SIZE_MASK)

#define SCANCODE_BUFFER_ISFULL (SCANCODE_BUFFER_NEXTLOC(scancode_buffer_end) \
				== scancode_buffer_begin)

#define SCANCODE_BUFFER_ISEMPTY (scancode_buffer_begin == scancode_buffer_end)


void
keyboard_init();

#endif /* __KEYBOARD_H__ */
