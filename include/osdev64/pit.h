#ifndef JEP_PIT_H
#define JEP_PIT_H

// PIT Interface
//
// This interface contains functions and datatypes for controlling the
// programmable interval timer (PIT). The PIT is a counter that is capable
// of generating a timer IRQ at regular intervals which can be used to
// preempt tasks, temporarily delay execution, and keep track of elapsed
// time intervals.

#include "osdev64/axiom.h"

/**
 * Initializes the PIT.
 * This must be called before any other functions in this interface.
 */
void k_pit_init();


/**
 * Waits for at least the specified number of PIT ticks.
 * This is a form of busy waiting, since the task that calls this function
 * enters a loop until the global tick count has reached the specified
 * threshold.
 *
 * Params:
 *   uint64_t - the number of ticks to wait
 */
void k_pit_wait(uint64_t);

#endif