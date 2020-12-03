#ifndef JEP_CORE_H
#define JEP_CORE_H

#include <stdint.h>

#include <efi.h>
#include <efilib.h>


// The memory map
typedef struct k_mem_map {
  UINTN map_size;
  EFI_MEMORY_DESCRIPTOR* buffer;
  UINTN key;
  UINTN desc_size;
  UINT32 version;
}k_mem_map;


/**
 * Initializes UEFI services.
 *
 * Params:
 *   EFI_HANDLE - the UEFI application image
 *   EFI_SYSTEM_TABLE* - the UEFI system table
 */
void k_uefi_init(EFI_HANDLE, EFI_SYSTEM_TABLE*);

/**
 * Terminates the boot services provided by UEFI.
 * This must be done before setting up a GDT, IDT, etc.
 *
 * Params:
 *   EFI_SYSTEM_TABLE* - the UEFI system table
 */
void k_uefi_exit();

/**
 * Gets the memory map.
 * This must be called immediately before k_uefi_exit.
 */
void k_uefi_get_mem_map();

/**
 * Gets the RSDP provided by UEFI firmware.
 * The RSDP is returned as an unsigned integer representing the beginning
 * memory address of the RSDP structure.
 * 
 * Params:
 *   unsigned char** rsdp - 
 *
 * Returns:
 *   int - 1 if ACPI version < 2, 2 if ACPI version >= 2, or 0 on failure.
 */
int k_uefi_get_rsdp(unsigned char**);

#endif