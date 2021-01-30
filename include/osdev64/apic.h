#ifndef JEP_APIC_H
#define JEP_APIC_H

#include <stdint.h>

// APIC Interface
// The APIC interface consists of two types of APICs:
// local APICs, which handle interrupts for individual cores,
// and I/O APICs, which handle interrupts from external devics.


/**
 * Initializes the APIC interface.
 * This function must be called before calling any other functions in this
 * interface.
 * It reads the MADT to find the physical addresses of the local and I/O
 * APIC after which it maps their addresses into virtual address space.
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

void k_apic_wait(uint64_t ticks);

#endif