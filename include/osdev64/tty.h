#ifndef JEP_TTY_H
#define JEP_TTY_H

// Kernel TTY Interface
//
// Functions and data types for a kernel-mode teletype (TTY) terminal
// emulator.

#include "osdev64/axiom.h"


/**
 * Initializes the kernel-mode TTY terminal emulator.
 * This must be called before any other functions in this interface.
 */
void k_tty_init();


/**
 * Temporary function to allow other processes to get a handle to
 * stdout.
 */
void* k_tty_get_shell_stdout();


#endif