/** @file console.h 
 *
 *  @brief Console Driver implementation definitions.
 *
 *  @author Anshuman P.Kanetkar (apk)
 * 
 */
#ifndef __CONSOLE_H__
#define __CONSOLE_H__
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

// Console display macros
#define FB_LOC_CHAR(row, col) ((char *)(CONSOLE_MEM_BASE + \
					2 * ((row) * CONSOLE_WIDTH + (col))))

#define FB_LOC_ATTRIB(row, col) (FB_LOC_CHAR(row, col)+ 1)

#define CONS_DEFAULT_COLOR (FGND_WHITE | BGND_BLACK)
#define CONS_CHAR_SPC ' '
#define CONS_CHAR_BKSPC '\b'
#define CONS_CHAR_CR '\r'
#define CONS_CHAR_LF '\n'

#define CONS_CURSOR_VISIBLE 1
#define CONS_CURSOR_HIDDEN 0

#define CONS_IS_VALID_COLOR(c) (((c) & (~0UL << 8)) == 0)

void
console_init();

void 
test_console(void);


#endif /* __CONSOLE_H__ */
