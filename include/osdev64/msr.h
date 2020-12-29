#ifndef JEP_MSR_H
#define JEP_MSR_H

// MSR Interface
// Functions and data types for reading and writing MSRs.


#include <stdint.h>

/**
 * Reads the value of the IA32_PAT MSR.
 *
 * Returns:
 *   uint64_t - the 64 bits of the IA32_PAT MSR
 */
uint64_t k_get_pat();

/**
 * Reads the value in the IA32_MTRRCAP MSR.
 */
uint64_t k_get_mtrrcap();

#endif