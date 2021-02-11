#ifndef JEP_FIRMWARE_H
#define JEP_FIRMWARE_H

// Firmware Interface
//
// This interface contains functions and data types for interacting with
// the firmware. This is used for things like obtaining a map of physical
// memory and locating a framebuffer for graphical output.


#include "osdev64/axiom.h"


/**
 * A map of physical memory.
 */
typedef struct k_mem_map {
  UINTN map_size;
  EFI_MEMORY_DESCRIPTOR* buffer;
  UINTN key; // required to exit boot services
  UINTN desc_size;
  UINT32 version;
}k_mem_map;


/**
 * Graphics mode information.
 */
typedef struct k_graphics {
  UINT32 width;  // horizontal resolution
  UINT32 height; // vertical resolution
  UINT32 pps;    // pixels per scan line
  EFI_GRAPHICS_PIXEL_FORMAT format;
  UINTN size;    // frame buffer size
  uint64_t base; // frame buffer base address
}k_graphics;


/**
 * Initializes the firmware service interface.
 * This must be called before any other functions in this interface.
 *
 * Params:
 *   EFI_HANDLE - a UEFI application image
 *   EFI_SYSTEM_TABLE* - a UEFI system table
 */
void k_firmware_init(EFI_HANDLE, EFI_SYSTEM_TABLE*);


/**
 * Terminates the firmware boot services.
 * This must be done before setting up a GDT, IDT, etc.
 *
 * Returns:
 *   int - 1 on successful exit, or 0 on failure
 */
int k_firmware_exit();


#endif