/** @file timer_wrapper.S
 *  @brief Wrapper for calling the timer tick handler
 *
 *
 *  @author Anshuman P.Kanetkar (apk)
 */
	
.global timer_wrapper

/* @brief The wrapper function
 */
timer_wrapper:
	// Push all general purpose registers
	pusha
	// Call the timer interrupt processing routine
	call timer_interrupt_handler
	// Pop all general purpose registers
	popa
	// Return from hardware interrupt
	iret
