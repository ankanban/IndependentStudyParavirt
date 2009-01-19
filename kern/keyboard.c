/** @file keyboard.c 
 *
 *  @brief Keyboard Driver implementation.
 *
 * The keyboard driver uses a 1KB circular scancode buffer
 * The interrupt handler writes into it, and readchar reads from it.
 * scancode_buffer_begin and scancode_buffer_end point to the current
 * buffer limits. the buffer is empty if they are equal, and it is 
 * full when 'begin' is the next position from 'end'.
 * Since the two variables are written to from different contexts
 * ('begin' from the interrupt handler, and 'end' from readchar), there should
 * be no protection required for them.
 *
 *  @author Anshuman P.Kanetkar (apk)
 * 
 */
 
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
#include <kdebug.h>

// Keyboard driver data

/**
 * @brief Circular buffer for keyboard scancodes
 */
static unsigned char scancode_buffer[SCANCODE_BUFFER_SIZE];
static unsigned int scancode_buffer_begin = 0;
static unsigned int scancode_buffer_end = 0;

/**
 * @brief Set up the IDT trap descriptor for the
 * keyboard handler.
 */
void
keyboard_init()
{
  // Set keyboard handler
  set_idt_entry(KEY_IDT_ENTRY,
		KEYBOARD_PORT,
  		keyboard_wrapper);
  
}

/**
 * @brief Add a scancode into the circular buffer,
 * incrementing the end of buffer.
 */
static inline void
keyb_add_scancode(char sc)
{
  if (SCANCODE_BUFFER_ISFULL) {
    return;
  }

  scancode_buffer[scancode_buffer_end] = sc;   
  scancode_buffer_end = SCANCODE_BUFFER_NEXTLOC(scancode_buffer_end);
}

/**
 * @brief Remove a scancode from the circular buffer.
 * @returns Scancode 
 */
static inline int
keyb_remove_scancode()
{
  if (SCANCODE_BUFFER_ISEMPTY) {
    return -1;
  }
  int sc = scancode_buffer[scancode_buffer_begin];
  scancode_buffer_begin = SCANCODE_BUFFER_NEXTLOC(scancode_buffer_begin);
  kdinfo("Found character: 0x%x", sc);
  return sc;
}


/**
 * @brief Interrupt handler for the keyboard, reads a scancode
 * from the port, adds it to the buffer if possible, and acks the
 * interrupt.
 */
void 
keyboard_interrupt_handler(void)
{
  unsigned char scan_code = inb(KEYBOARD_PORT);


  keyb_add_scancode(scan_code);


  outb(INT_CTL_PORT, INT_ACK_CURRENT);
  
  /* Enable event channel */
  
}

/**
 * @brief Reads a character from the keyyboard buffer 
 * @returns -1 if no chharacter read, the character code otherwise
 */
int
readchar(void)
{
  
  int sc = -1;

  /* Try to get a scancode from the buffer */
  sc = keyb_remove_scancode();
  
  /* Check if the buffer was empty */
  if (sc == -1) {
    return -1;
  }

  /* feed the process_scancode state machine */
  kh_type augch = process_scancode(sc);

  /* If no data, skip over this scancode */
  if (!KH_HASDATA(augch)) {
    return -1;
  }
  
  
  /* Found a valid scancode */
  if (KH_ISMAKE(augch)) {
    kdinfo("Reading make char: 0x%02x:%c", KH_GETCHAR(augch));
    return KH_GETCHAR(augch);
  }

  /* Skip over release events */
  kdinfo("Release event?");
  return -1;
}

