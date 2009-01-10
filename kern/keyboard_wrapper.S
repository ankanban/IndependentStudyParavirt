/** @file keyboard_wrapper.S
 *  @brief Wrapper for calling the keyboard handler
 *
 *
 *  @author Anshuman P.Kanetkar (apk)
 */

.global keyboard_wrapper

/* @brief The wrapper function
 */
keyboard_wrapper:
	// Push all general purpose registers
	pusha
	// Call the timer interrupt processing routine

	call keyboard_interrupt_handler
	// Pop all general purpose registers
	popa
	// Return from hardware interrupt
	iret
