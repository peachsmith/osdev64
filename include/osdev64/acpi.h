#ifndef JEP_ACPI_H
#define JEP_ACPI_H

/**
 * Initializes the ACPI table reader.
 * This must be called before any other functions in this interface.
 */
void k_acpi_init();


/**
 * Reads the Multiple APIC Description Table (MADT).
 * The goal of this function is to extract the base address of
 * the local APIC and IO APIC.
 * It also maps those base addresses into virtual address space.
 */
void k_acpi_read_madt();


/**
 * This function writes the contents of the MADT to some output stream.
 * It is intended to be used for debugging. The actual output destination
 * is undefined.
 */
void k_acpi_print_madt();


#endif