#ifndef JEP_MSR_H
#define JEP_MSR_H

// MSR Interface
// Functions and data types for reading and writing MSRs.


#include <stdint.h>

// The PAT
#define IA32_PAT (uint64_t)0x277

// MTRRCAP (contains information about the MTRRs)
#define IA32_MTRRCAP (uint64_t)0xFE

// Fixed Range MTRRs
#define IA32_MTRR_FIX64K_00000 (uint64_t)0x250
#define IA32_MTRR_FIX16K_80000 (uint64_t)0x258
#define IA32_MTRR_FIX16K_A0000 (uint64_t)0x259
#define IA32_MTRR_FIX4K_C0000  (uint64_t)0x268
#define IA32_MTRR_FIX4K_C8000  (uint64_t)0x269
#define IA32_MTRR_FIX4K_D0000  (uint64_t)0x26A
#define IA32_MTRR_FIX4K_D8000  (uint64_t)0x26B
#define IA32_MTRR_FIX4K_E0000  (uint64_t)0x26C
#define IA32_MTRR_FIX4K_E8000  (uint64_t)0x26D
#define IA32_MTRR_FIX4K_F0000  (uint64_t)0x26E
#define IA32_MTRR_FIX4K_F8000  (uint64_t)0x26F

// Variable Range MTRRs
#define IA32_MTRR_PHYSBASE0 (uint64_t)0x200
#define IA32_MTRR_PHYSMASK0 (uint64_t)0x201
#define IA32_MTRR_PHYSBASE1 (uint64_t)0x202
#define IA32_MTRR_PHYSMASK1 (uint64_t)0x203
#define IA32_MTRR_PHYSBASE2 (uint64_t)0x204
#define IA32_MTRR_PHYSMASK2 (uint64_t)0x205
#define IA32_MTRR_PHYSBASE3 (uint64_t)0x206
#define IA32_MTRR_PHYSMASK3 (uint64_t)0x207
#define IA32_MTRR_PHYSBASE4 (uint64_t)0x208
#define IA32_MTRR_PHYSMASK4 (uint64_t)0x209
#define IA32_MTRR_PHYSBASE5 (uint64_t)0x20A
#define IA32_MTRR_PHYSMASK5 (uint64_t)0x20B
#define IA32_MTRR_PHYSBASE6 (uint64_t)0x20C
#define IA32_MTRR_PHYSMASK6 (uint64_t)0x20D
#define IA32_MTRR_PHYSBASE7 (uint64_t)0x20E
#define IA32_MTRR_PHYSMASK7 (uint64_t)0x20F
#define IA32_MTRR_PHYSBASE8 (uint64_t)0x210
#define IA32_MTRR_PHYSMASK8 (uint64_t)0x211
#define IA32_MTRR_PHYSBASE9 (uint64_t)0x212
#define IA32_MTRR_PHYSMASK9 (uint64_t)0x213


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

/**
 * Gets the value of an MSR specified by the argument.
 *
 * Params:
 *   uint64_t - the register address of the MSR
 *
 * Returns:
 *   uint64_t - the contents of the MSR.
 */
uint64_t k_get_msr(uint64_t);

#endif