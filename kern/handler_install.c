/** @file handler_install.c 
 *
 *  @brief Interrupt/Exception initialization routines
 *
 *  @author Anshuman P.Kanetkar (apk)
 * 
 */

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
#include <interrupt_wrappers.h>
#include <handler_install.h>
#include <timer.h>
#include <keyboard.h>
#include <console.h>
#include <kernel_exceptions.h>
#include <kdebug.h>

void
set_exception_entry(int index,
		    void (*handler)(void))
{
  idt_gate_desc_t * idt_entry = (idt_gate_desc_t *)idt_base();
  unsigned short *handler_offset = (unsigned short *)&handler;
  
  idt_entry += index;

  idt_entry->unused = 0;
  idt_entry->offset_low = handler_offset[0];
  idt_entry->offset_high = handler_offset[1];
  idt_entry->flags = IDT_FLAGS_INTR_GATE ;
  idt_entry->seg_sel = SEGSEL_KERNEL_CS;
 
}

/**
 * @brief Set the idt entry at the given index
 */
void 
set_syscall_entry(int index, 
		  void (*handler)(void))
{
  idt_gate_desc_t * idt_entry = (idt_gate_desc_t *)idt_base();
  unsigned short *handler_offset = (unsigned short *)&handler;

  kdinfo("idt_base: %p", idt_entry);
  kdinfo("handler: %p", handler);

  idt_entry += index;

  idt_entry->unused = 0;
  idt_entry->offset_low = handler_offset[0];
  idt_entry->offset_high = handler_offset[1];
  idt_entry->flags = IDT_FLAGS_SYSCALL_TRAP_GATE;
  idt_entry->seg_sel = SEGSEL_KERNEL_CS;
  
  kdinfo("off_lo:0x%04x, off_hi:0x%04x, segsel:0x%04x, flags:0x%02x\n",
	 idt_entry->offset_low,
	 idt_entry->offset_high,
	 idt_entry->seg_sel,
	 idt_entry->flags);
   
}


/**
 * @brief Set the idt entry at the given index
 */
void 
set_idt_entry(int index, 
	      void (*handler)(void))
{
  idt_gate_desc_t * idt_entry = (idt_gate_desc_t *)idt_base();
  unsigned short *handler_offset = (unsigned short *)&handler;

  kdinfo("idt_base: %p", idt_entry);
  kdinfo("handler: %p", handler);

  idt_entry += index;

  idt_entry->unused = 0;
  idt_entry->offset_low = handler_offset[0];
  idt_entry->offset_high = handler_offset[1];
  idt_entry->flags = IDT_FLAGS_TRAP_GATE;
  idt_entry->seg_sel = SEGSEL_KERNEL_CS;
  
  kdinfo("off_lo:0x%04x, off_hi:0x%04x, segsel:0x%04x, flags:0x%02x",
	 idt_entry->offset_low,
	 idt_entry->offset_high,
	 idt_entry->seg_sel,
	 idt_entry->flags);
   
}

/**
 * @brief Installs the timer tick handler, and initializes
 * timer, keyboard and console drivers.
 */
int 
handler_install(void (*tickback_upper)(unsigned int),
		void (*tickback_lower)(void))
{
  
  exceptions_init();

  timer_init(tickback_upper,
	     tickback_lower);
 
  keyboard_init();

  console_init();

  return 0;
}


