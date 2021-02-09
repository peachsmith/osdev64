#ifndef JEP_PIT_H
#define JEP_PIT_H

#include "osdev64/axiom.h"

/**
 * Initializes the PIT.
 * 
 */
void k_pit_init();


/**
 * Waits for at least the specified number of PIT ticks.
 * 
 * Params:
 *   uint64_t - the number of ticks to wait
 */
void k_pit_wait(uint64_t);

#endif