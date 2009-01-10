
/** @file handler_install.h 
 *
 *  @brief Driver implementation definitions.
 *
 *  @author Anshuman P.Kanetkar (apk)
 * 
 */
#ifndef __HANDLER_INSTALL_H__
#define __HANDLER_INSTALL_H__

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

/*
 * @brief Gate descriptor
 */
typedef struct {
  unsigned short offset_low;
  unsigned short seg_sel;
  unsigned char unused;
  unsigned char flags;
  unsigned short offset_high;
} __attribute__((__packed__)) idt_gate_desc_t;

#define IDT_FLAGS_TRAP_GATE 0x8f
#define IDT_FLAGS_SYSCALL_TRAP_GATE 0xef
#define IDT_FLAGS_INTR_GATE 0x8e
#define IDT_FLAGS_TASK_GATE 0x85


#define IDT_NUM_NMI 32

#define lsb(x) ((x) & 0xffUL)
#define msb(x) ((((x) & 0xff00UL) >> 8) & 0xffUL)

void
set_exception_entry(int index,
		    void (*handler)(void));

void 
set_idt_entry(int index, 
	      void (*handler)(void));

void 
set_syscall_entry(int index, 
		  void (*handler)(void));


#endif /* __HANDLER_INSTALL_H__ */
