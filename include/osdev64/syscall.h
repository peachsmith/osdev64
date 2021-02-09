#ifndef JEP_SYSCALL_H
#define JEP_SYSCALL_H

#include "osdev64/axiom.h"

#define SYSCALL_START 1
#define SYSCALL_STOP 2
#define SYSCALL_SLEEP 3


// used for debugging
#define SYSCALL_FACE 0xFACE

/**
 * The ISR used for initiating system calls.
 */
void k_syscall_isr();


/**
 * The handler function for syscalls. This function is called by the syscall
 * ISR. The first argument is an ID that determines which syscall is being
 * performed. The second argument is the current task's register stack.
 * The remaining arguments are any data that is needed to perform the syscall.
 *
 * Params:
 *   k_regn - the ID of the syscall
 *   k_regn* - a pointer to the current task's register stack
 *   k_regn - the first data argument
 *   k_regn - the second data argument
 *   k_regn - the third data argument
 *   k_regn - the fourth data argument
 */
k_regn* k_syscall(
  k_regn id,
  k_regn* regs,
  k_regn data1,
  k_regn data2,
  k_regn data3,
  k_regn data4
);


/**
 * Calls the FACE syscall.
 * Used for testing and debugging.
 * The behavior is completely undefined.
 */
void k_syscall_face(k_regn);


/**
 * Stops the currently running task.
 */
void k_syscall_stop();


#endif