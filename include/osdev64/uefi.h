#ifndef JEP_UEFI_H
#define JEP_UEFI_H

#include <stdint.h>

#include <efi.h>
#include <efilib.h>


// memory map
typedef struct k_mem_map {
  UINTN map_size;
  EFI_MEMORY_DESCRIPTOR* buffer;
  UINTN key;
  UINTN desc_size;
  UINT32 version;
}k_mem_map;


// graphics information
typedef struct k_graphics {
  UINT32 width;
  UINT32 height;
  UINT32 pps; // pixels per scan line
  EFI_GRAPHICS_PIXEL_FORMAT format;
  UINTN size; // frame buffer size
  uint64_t base; // frame buffer base address
}k_graphics;


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

/**
 * Gets the information needed to render graphics to the screen.
 * This is done using the graphics output protocol provided by the UEFI
 * firmware.
 *
 * Params:
 *   k_graphics* - a pointer to a structure to hold the graphics information.
 */
void k_uefi_get_graphics(k_graphics*);

/**
 * Loads a bitmap font into memory.
 * We currently use the zap-vga16 font from
 * https://www.zap.org.au/projects/console-fonts-zap
 *
 * This font supports 256 characters, where each character is 16 bytes.
 * So the total font data loaded into memory by this function is 4096 bytes.
 *
 * Returns:
 *   unsigned char* - a pointer to the font data
 */
unsigned char* k_uefi_load_font();

#endif