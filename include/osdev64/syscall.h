#ifndef JEP_SYSCALL_H
#define JEP_SYSCALL_H

#include "osdev64/axiom.h"

#include "klibc/stdio.h"

#define SYSCALL_START 1
#define SYSCALL_STOP 2
#define SYSCALL_SLEEP_SYNC 3
#define SYSCALL_SLEEP_TICK 4
#define SYSCALL_WRITE 5
#define SYSCALL_READ 6


// used for debugging
#define SYSCALL_FACE 0xFACE

/**
 * The ISR used for initiating system calls.
 */
void k_syscall_isr();


/**
 * The handler function for syscalls. This function is called by the syscall
 * ISR. The first argument is an ID that determines which syscall is being
 * performed. The remaining arguments are any data that is needed to perform
 * the syscall.
 *
 * Params:
 *   k_regn - the ID of the syscall
 *   k_regn - the first data argument
 *   k_regn - the second data argument
 *   k_regn - the third data argument
 *   k_regn - the fourth data argument
 */
k_regn k_syscall(
  k_regn id,
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

/**
 * Puts the current task to sleep for at least the specified number
 * of timer ticks.
 *
 * Params:
 *   uint64_t - the number of ticks to sleep
 */
void k_syscall_sleep(uint64_t);


/**
 * Writes the contents of a buffer to a file.
 *
 * Params:
 *   FILE* - the file pointer
 *   char* - the source buffer
 *   size_t - the number of bytes to write
 * 
 * Returns:
 *   k_regn - the number of bytes written
 */
k_regn k_syscall_write(FILE*, char*, size_t);


/**
 * Reads the contents of a file into a buffer.
 *
 * PArams:
 *   FILE* - the file pointer
 *   char* - the destination buffer
 *   size_t - the number of bytes to read
 * 
 * Returns:
 *   k_regn - the number of bytes readI
 */
k_regn k_syscall_read(FILE*, char*, size_t);


#endif