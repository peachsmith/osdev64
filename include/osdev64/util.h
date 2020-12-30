#ifndef JEP_UTIL_H
#define JEP_UTIL_H

#include <stdint.h>

/**
 * Finds the nearest multiple of 4096 below the specified address.
 * The resulting address will be less than or equal to the address
 * specified by the argument.
 *
 * Params:
 *   uint64_t - a 64-bit address
 *
 * Returns:
 *   uint64_t - the closest multiple of 4096 to the address
 */
uint64_t k_align_4k(uint64_t);

#endif