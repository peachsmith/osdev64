#ifndef JEP_CPUID_H
#define JEP_CPUID_H

// CPUID Interface
// Functions and data types for executing the CPUID instruction.

#include <stdint.h>

/**
 * Executes the CPUID instruction.
 * This function returns the value that CPUID placed in RAX.
 *
 * Params:
 *   uint64_t - the input for CPUID
 *
 * Returns:
 *   uint64_t - the result of CPUID
 */
uint64_t k_cpuid_rax(uint64_t);

/**
 * Executes the CPUID instruction.
 * This function returns the value that CPUID placed in RDX.
 *
 * Params:
 *   uint64_t - the input for CPUID
 *
 * Returns:
 *   uint64_t - the result of CPUID
 */
uint64_t k_cpuid_rdx(uint64_t);

/**
 * Executes the CPUID instruction to get the vendor identification string.
 * The string is placed in a buffer that is passed to this function.
 * The string is 12 characters, so the buffer should have at least that
 * much memory available.
 * 
 * Params:
 *   char* - buffer with space for at least 12 characters
 */
void k_cpuid_vendor(char*);

#endif