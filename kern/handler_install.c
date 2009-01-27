/** @file handler_install.c 
 *
 *  @brief Interrupt/Exception initialization routines
 *
 *  @author Anshuman P.Kanetkar (apk)
 * 
 */

//#include <p1kern.h>
#define FORCE_DEBUG
#define DEBUG_LEVEL KDBG_INFO
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
#include <kernel_syscall.h>
#include <assert.h>
#include <kdebug.h>

#include <xen/callback.h>
#include <hypervisor.h>
#include <xen_events.h>

trap_info_t ex_trap_table[32] = { {0,0,0,0}, };
int num_ex;

trap_info_t sys_trap_table[64] = { {0,0,0,0}, };
int num_sys;

void xen_init_console(void);

void 
hypervisor_callback(void);

void
failsafe_callback(void);


void
set_exception_entry(int index,
		    void (*handler)(void))
{

  trap_info_t * ti = &ex_trap_table[num_ex++];

  ti->vector = index;
  ti->flags = 0;
  ti->cs = SEGSEL_KERNEL_CS;;
  ti->address = (unsigned long)handler;

 
  //TI_SET_DPL(ti, 3);
  //TI_SET_IF(ti, 1);
  
  
  /*  
  idt_gate_desc_t * idt_entry = (idt_gate_desc_t *)idt_base();
  unsigned short *handler_offset = (unsigned short *)&handler;
  
  idt_entry += index;

  idt_entry->unused = 0;
  idt_entry->offset_low = handler_offset[0];
  idt_entry->offset_high = handler_offset[1];
  idt_entry->flags = IDT_FLAGS_INTR_GATE ;
  idt_entry->seg_sel = SEGSEL_KERNEL_CS;
  */
}

/**
 * @brief Set the idt entry at the given index
 */
void 
set_syscall_entry(int index, 
		  void (*handler)(void))
{

  trap_info_t * ti = &sys_trap_table[num_sys++];

  ti->vector = index;
  ti->flags = 0;
  ti->cs = SEGSEL_KERNEL_CS;;
  ti->address = (unsigned long)handler;

  /* Ring 3 is allowed to call the interrupt */
  TI_SET_DPL(ti, 3);
  /* Syscalls don't set the event mask */
  //TI_SET_IF(ti, 1);

  /*
  idt_gate_desc_t * idt_entry = (idt_gate_desc_t *)idt_base();
  unsigned short *handler_offset = (unsigned short *)&handler;

  kdinfo("idt_base: %p", idt_entry);
  kdinfo("handler: %p", handler);

  idt_entry += index;

  Idtvom_entry->unused = 0;
  idt_entry->offset_low = handler_offset[0];
  idt_entry->offset_high = handler_offset[1];
  idt_entry->flags = IDT_FLAGS_SYSCALL_TRAP_GATE;
  idt_entry->seg_sel = SEGSEL_KERNEL_CS;
  
  kdinfo("off_lo:0x%04x, off_hi:0x%04x, segsel:0x%04x, flags:0x%02x\n",
	 idt_entry->offset_low,
	 idt_entry->offset_high,
	 idt_entry->seg_sel,
	 idt_entry->flags);
  */
}


/**
 * @brief Set the idt entry at the given index
 */
void 
set_idt_entry(int index, 
	      int port,
	      void (*handler)(void))
{
  int ret = 0;

  evtchn_op_t ev_array[2];
  evtchn_op_t * ev;

  memset(&ev_array, 0, sizeof(ev_array));
  
  ev = &ev_array[0];

  ev->cmd = EVTCHNOP_bind_virq;

  ev->u.bind_virq.virq = index;
  ev->u.bind_virq.vcpu = 0;
  ev->u.bind_virq.port = port;
  
  
  ret = HYPERVISOR_event_channel_op(EVTCHNOP_bind_virq,
				    ev);

  assert(ret == 0);

  /*
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
  */
}


int
hypervisor_init(void)
{
  int ret = 0;
  /* use older Xen interface 3.0.2*/

  ret = HYPERVISOR_set_callbacks(SEGSEL_KERNEL_CS,
				 (unsigned long)hypervisor_callback,
				 SEGSEL_KERNEL_CS,
				 (unsigned long)failsafe_callback);
  
  assert(ret == 0);

  kdinfo("Setup hypervisor callbacks");

  return ret;
}

/**
 * @brief Installs the timer tick handler, and initializes
 * timer, keyboard and console drivers.
 */
int 
handler_install(void (*tickback_upper)(unsigned int),
		void (*tickback_lower)(void))
{
 
  int ret = 0;

  hypervisor_init();

  num_ex = 0;
  num_sys = 0;
 
  exceptions_init();

  /* Register the exceptions tabe with Xen */
  ret = HYPERVISOR_set_trap_table(ex_trap_table);
    
  assert(ret == 0);

  syscall_init();

  /* Register the syscall table with Xen */
  ret = HYPERVISOR_set_trap_table(sys_trap_table);
    
  assert(ret == 0);


  return 0;
}


