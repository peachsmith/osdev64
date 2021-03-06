#ifndef JEP_INTERRUPTS_H
#define JEP_INTERRUPTS_H

#include <stdint.h>

/**
 * Installs an interrupt service routine (ISR) in the IDT.
 * The first arguments is a function to handle an interrupt, and the
 * second argument is the interrupt number, which can range from 32 to 255.
 * Interrupts 0 through 31 are reserved by the CPU for exceptions.
 *
 * Params:
 *   void (isr)() - the address of the ISR function
 *   int - the interrupt number
 */
void k_install_isr(void (isr)(), int i);

#endif