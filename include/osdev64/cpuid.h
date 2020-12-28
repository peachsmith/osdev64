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

#endif