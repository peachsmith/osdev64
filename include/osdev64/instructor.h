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