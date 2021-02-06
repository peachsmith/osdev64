#ifndef JEP_INSTRUCTOR_H
#define JEP_INSTRUCTOR_H

// Functions that use assembly instructions that don't
// have C

#include "osdev64/axiom.h"

/**
 * Writes a byte to an I/O port.
 *
 * Params:
 *   uint16_t - an I/O port
 *   uint8_t - the byte to be written
 */
void k_outb(uint16_t port, uint8_t val);


/**
 * Reads a byte from an I/O port.
 *
 * Params:
 *   uint16_t - an I/O port
 *
 * Returns:
 *   uint8_t - a byte
 */
uint8_t k_inb(uint16_t port);


/**
 * Disables interrupts.
 */
void k_disable_interrupts();


/**
 * Enabled interrupts.
 */
void k_enable_interrupts();

/**
 * Executes the XCHG instruction to swap some value with a location
 * in memory.
 * The value is expected to be 64 bits, and the memory location is
 * expected to have at least 64 bits of readable and writable memory
 * available.
 *
 * Params:
 *   k_regn - a value to put in a memory location
 *   k_regn* - a memory location to receive the value
 *
 * Returns:
 *   k_regn - the value that was previously at the memory location
 */
k_regn k_xchg(k_regn, k_regn*);


/**
 * Executes the BTS instruction to test and set a bit.
 * The first argument specifies which bit to test and set,
 * and the second arguments specifies a location in memory
 * that contain the bit in question.
 * It is assumed that the memory location is readable and
 * writable.
 *
 * Params:
 *   k_regn - a number representing the bit to set
 *   k_regn* - a location in memory containing the bit to set
 *
 * Returns:
 *   k_regn - the previous value of the bit
 */
k_regn k_bts(k_regn, k_regn*);


/**
 * Executes the BTR instruction to test and clear a bit.
 * The first argument specifies which bit to test and set,
 * and the second arguments specifies a location in memory
 * that contain the bit in question.
 * It is assumed that the memory location is readable and
 * writable.
 *
 * Params:
 *   k_regn - a number representing the bit to clear
 *   k_regn* - a location in memory containing the bit to clear
 *
 * Returns:
 *   k_regn - the previous value of the bit
 */
k_regn k_btr(k_regn, k_regn*);


/**
 * Executes the BTS instruction to set bit 0 of a 64-bit value.
 * If the carry flag is set after executing the BTS instruction,
 * then this procedure loops until bit 0 is no longer set, at
 * which point it makes another attempt to set bit 0.
 *
 * Params:
 *   k_regn - a number representing the bit to set
 *   k_regn* - a location in memory containing the bit to set
 */
void k_bts_wait(k_regn, k_regn*);


/**
 * Executes the XADD instruction to add the first argument to the value
 * pointed to by the second argument.
 *
 * Params:
 *   int64_t - the amount to add to the destination
 *   int64_t* - a pointer to the value to be added
 */
int32_t k_xadd(int64_t, int64_t*);


/**
 * Executes the XADD instruction to add the first argument to the value
 * pointed to by the second argument. This function is like k_xadd,
 * but this function loops until the value pointed to by the second
 * argument is greater than or equal to zero.
 *
 * Params:
 *   int64_t - the amount to add to the destination
 *   int64_t* - a pointer to the value to be added
 */
int32_t k_xadd_wait(int64_t, int64_t*);


/**
 * Causes an exception.
 * This typically causes a divide error by attempting to divide by 0,
 * but it may be changed to do anything at any time.
 * This is mainly used to test the ISRs that handle exceptions.
 */
void k_cause_exception();


/**
 * Does some stuff.
 * Currently, this is used to raise interrupts to debug ISRs.
 */
void k_nonsense();

#endif