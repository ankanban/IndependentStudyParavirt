/** @file lib/simics.h
 *  @brief Simics interface (kernel side)
 *  @author matthewj S2008
 */

#ifndef LIB_SIMICS_H
#define LIB_SIMICS_H

/** @brief Prints a string to the simics console */
void SIM_puts(char *arg);
/** @brief Breakpoint */
void SIM_break();
/** @brief Halt the simulation */
void SIM_halt();

/** @brief Beep under simics. */
void SIM_beep();

/** @brief Register a user process with the CS410 simics debugging code */
void SIM_register_user_proc( void *cr3, char *fname );
/** @brief Register a user process as a fork of another.
 *
 *  Hhandy if fork() is oblivious of the program's name.
 */
void SIM_register_user_from_parent( void *child_cr3, void *parent_cr3 );
/** @brief Unregister a user process from the CS410 simics debugging code */
void SIM_unregister_user_proc( void *cr3 );
/** @brief Inform CS410 simics debugging code of a context switch */
void SIM_switch( unsigned int cr3 );

/** @brief Convenience wrapper around sprintf for simics */
void SIM_printf(char *fmt, ...);

/** @brief Notify simics that we have booted.
 *
 *  This is done for you in 410kern/entry.c
 */
void SIM_notify_bootup(char *);

/* "Compatibility mode" for old code */
#define MAGIC_BREAK SIM_break()
#define lprintf(...) SIM_printf(__VA_ARGS__)

#endif /* !LIB_SIMICS_H */
