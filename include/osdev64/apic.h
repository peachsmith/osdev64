#ifndef JEP_APIC_H
#define JEP_APIC_H


// APIC Interface
//
// This interface contains functions and data types for interacting with
// the advanced programmable interrupt controllers (APIC).
// There are two types of APICs:
// 1. Local APICs, which receives IRQs coming into individual cores,
// 2. I/O APICs, which receives IRQs from devices and passes them along
//    to the local APICs.


#include "osdev64/axiom.h"


/**
 * Initializes the APIC interface.
 * This must be called before any other functions in this interface.
 */
void k_apic_init();


/**
 * Checks to see if the local and I/O APICs are available.
 * If either the local APIC or I/O APIC are not detected, this function
 * returns 0.
 *
 * Returns:
 *   int - 1 if both local and I/O APICs are present, otherwise 0.
 */
int k_apic_available();


//============================================================
// Local APIC
//============================================================

/**
 * Enables the current local apic.
 */
void k_lapic_enable();

/**
 * Gets the ID of the current APIC.
 *
 * Returns:
 *   uint64_t - the ID of the current APIC
 */
uint32_t k_lapic_get_id();

/**
 * Gets the version of the current APIC.
 * This should be a number ranging from 0x00 to 0x15
 *
 * Returns:
 *   uint64_t - the version of the current APIC
 */
uint32_t k_lapic_get_version();

/**
 * Gets the max LVT value for the current APIC - 1.
 *
 * Returns:
 *   uint64_t - max LVT - 1.
 */
uint32_t k_lapic_get_maxlvt();


//============================================================
// I/O APIC
//============================================================

/**
 * Configures the IRQ redirects in the I/O APIC.
 */
void k_ioapic_configure();

/**
 * Gets the version of the current I/O APIC.
 *
 * Returns:
 *   uint64_t - the version the I/O APIC.
 */
uint32_t k_ioapic_get_version();

/**
 * Gets the maximum redirect entries for the current I/O APIC.
 *
 * Returns:
 *   uint64_t - the maximum redirect entries for the I/O APIC.
 */
uint32_t k_ioapic_get_max_redirect();


#endif