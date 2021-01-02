#ifndef JEP_MTRR_H
#define JEP_MTRR_H

/**
 * Memory Type Range Registers (MTRR) Interface.
 * Functions and data types for reading the MTRRs.
 *
 */

;

#include <stdint.h>

/**
 * Prints the contents of all available MTRRs to some output stream.
 * It is intended to be used for debugging. The actual output destination
 * is undefined.
 */
void k_mtrr_print_all();

#endif