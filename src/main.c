#include "core.h"

#include "serial.h"
#include "descriptor.h"
#include "acpi.h"
#include "graphics.h"


void k_disable_interrupts();
void k_enable_interrupts();

// attempts to do something that results in an exception
void k_cause_exception();


/**
 * Used for debugging graphics stuff.
 *
 * Params:
 *   EFI_GRAPHICS_OUTPUT_PROTOCOL* - pointer to UEFI graphics protocol
 */
void k_do_graphics()
{

  // slope == 0
  k_draw_line(300, 200, 400, 200, 50, 230, 100); // green

  // slope < 1
  k_draw_line(300, 210, 400, 211, 255, 80, 80); // red
  k_draw_line(300, 220, 400, 227, 255, 80, 80); // red
  k_draw_line(300, 230, 400, 250, 255, 80, 80); // red

  // slope == 1
  k_draw_line(300, 250, 400, 350, 240, 220, 80); // yellow

  // slope > 1
  k_draw_line(300, 280, 400, 381, 200, 100, 200); // magenta
  k_draw_line(300, 290, 400, 397, 200, 100, 200); // magenta
  k_draw_line(300, 300, 400, 420, 200, 100, 200); // magenta

  // vertical line
  k_draw_line(300, 320, 300, 460, 50, 150, 240); // blue

  // slope < 0
  k_draw_line(300, 180, 400, 150, 10, 180, 180); // teal


  k_draw_line(300, 170, 200, 140, 255, 120, 20); // orange
  k_draw_line(300, 160, 200, 60, 150, 0, 150);  // purple
  k_draw_line(300, 150, 200, 20, 120, 100, 0);  // brown

  k_draw_line(280, 270, 180, 300, 255, 120, 20); // orange
  k_draw_line(280, 260, 180, 360, 150, 0, 150);  // purple
  k_draw_line(280, 250, 180, 370, 120, 100, 0);  // brown
}




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
  k_acpi_read();

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
  k_do_graphics();

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
