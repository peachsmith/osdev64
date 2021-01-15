#ifndef JEP_INTERRUPTS_H
#define JEP_INTERRUPTS_H

#include <stdint.h>

/**
 * Loads the IDT.
 */
void k_load_idt();

/**
 * Installs an interrupt service routine (ISR) in the IDT.
 * The first arguments is a function to handle an interrupt, and the
 * second argument is the interrupt number, which can range from 32 to 255.
 * Interrupts 0 through 31 are reserved by the CPU for exceptions.
 *
 * Params:
 *   uint64_t - the address of the ISR function
 *   int - the interrupt number
 */
void k_install_isr(uint64_t r, int i);

#endif