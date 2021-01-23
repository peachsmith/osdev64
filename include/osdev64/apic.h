#ifndef JEP_APIC_H
#define JEP_APIC_H

#include <stdint.h>

/**
 * Gets the ID of the current APIC.
 *
 * Returns:
 *   uint64_t - the ID of the current APIC
 */
uint64_t k_lapic_get_id();

/**
 * Gets the version of the current APIC.
 * This should be a number ranging from 0x00 to 0x15
 *
 * Returns:
 *   uint64_t - the version of the current APIC
 */
uint64_t k_lapic_get_version();

/**
 * Gets the max LVT value for the current APIC - 1.
 *
 * Returns:
 *   uint64_t - max LVT - 1.
 */
uint64_t k_lapic_get_maxlvt();


/**
 * Gets the version of the current I/O APIC.
 *
 * Returns:
 *   uint64_t - the version the I/O APIC.
 */
uint64_t k_ioapic_get_version();

/**
 * Gets the maximum redirect entries for the current I/O APIC.
 *
 * Returns:
 *   uint64_t - the maximum redirect entries for the I/O APIC.
 */
uint64_t k_ioapic_get_max_redirect();

#endif