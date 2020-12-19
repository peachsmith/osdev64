#include "core.h"

#include "serial.h"
#include "descriptor.h"
#include "acpi.h"
#include "graphics.h"
#include "console.h"


void k_disable_interrupts();
void k_enable_interrupts();


// attempts to do something that results in an exception
void k_cause_exception();


/**
 * Kernel entry point.
 */
EFI_STATUS
EFIAPI
efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE* systab)
{
  //==============================
  // BEGIN UEFI boot services

  // Initialize UEFI services.
  k_uefi_init(image, systab);

  // Initialize graphics
  k_graphics_init();

  // Print out some ACPI information.
  // k_acpi_read();

  k_console_init();

  // console text output
  k_console_puts("Hello, World!\n");
  k_console_puts("This is some text output.\n");

  // geometric primitives
  k_draw_rect(
    50, 50,
    50, 50,
    200, 120, 50);

  k_fill_rect(
    103, 50,
    50, 50,
    200, 120, 50
  );

  k_draw_triangle(
    100, 153,
    50, 153,
    75, 103,
    50, 120, 200
  );

  k_fill_triangle(
    153, 153,
    103, 153,
    128, 103,
    50, 120, 200
  );

  // Get the memory map.
  k_uefi_get_mem_map();

  // Terminate the boot services.
  k_uefi_exit();

  // END UEFI boot services
  //==============================



  // Initialize serial output for debugging.
  k_serial_com1_init();

  k_serial_com1_puts("UEFI boot services have been terminated.\n");

  // Draw some lines.
  // k_do_graphics();

  // Disable interrupts.
  k_disable_interrupts();

  // Load the GDT.
  k_load_gdt();

  // Load the IDT.
  k_load_idt();

  // Enable interrupts.
  k_enable_interrupts();

  // Test an exception handler.
  // k_cause_exception();

  k_serial_com1_puts("Initialization complete.\n");

  // The main loop.
  for (;;);

  // We should never get here.
  return EFI_SUCCESS;
}
