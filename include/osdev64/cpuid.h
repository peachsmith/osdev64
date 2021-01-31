#ifndef JEP_CPUID_H
#define JEP_CPUID_H

// CPUID Interface
// Functions and data types for executing the CPUID instruction.

#include "osdev64/axiom.h"

/**
 * Executes the CPUID instruction.
 * This function returns the value that CPUID placed in RAX.
 *
 * Params:
 *   k_regn - the input for CPUID
 *
 * Returns:
 *   k_regn - the result of CPUID
 */
k_regn k_cpuid_rax(k_regn);

/**
 * Executes the CPUID instruction.
 * This function returns the value that CPUID placed in RDX.
 *
 * Params:
 *   k_regn - the input for CPUID
 *
 * Returns:
 *   k_regn - the result of CPUID
 */
k_regn k_cpuid_rdx(k_regn);

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