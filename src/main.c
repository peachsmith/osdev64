#include "core.h"

#include "serial.h"
#include "descriptor.h"
#include "acpi.h"
#include "graphics.h"
#include "console.h"

#include "../klibc/include/stdio.h"


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

  // draw an outline of a rectangle
  k_draw_rect(
    250, 250,    // x, y
    50, 50,      // w, h
    200, 120, 50 // r, g, b
  );

  // draw a filled rectangle
  k_fill_rect(
    303, 250,    // x, y
    50, 50,      // w, h
    200, 120, 50 // r, g, b
  );

  // draw an outline of a triangle
  k_draw_triangle(
    300, 353,    // x1, y1
    250, 353,    // x2, y2
    275, 303,    // x3, y3
    50, 120, 200 // r, g, b
  );

  // draw a filled triangle
  k_fill_triangle(
    353, 353,    // x1, y1
    303, 353,    // x2, y2
    328, 303,    // x3, y3
    50, 120, 200 // r, g, b
  );


  // Initialize serial output on COM1
  k_serial_com1_init();

  // Initialize console output.
  k_console_init();

  // Write a "Hello, World" message to the console and to serial output.
  fputs("console output with fputs\n", stdout);
  fputs("serial output with fputs\n", stddbg);

  char my_char = 'J';
  char* my_str = "bagel";
  uint64_t my_long_hex = 0xDEAD0000BEEF0000;
  int my_n = -17;
  int my_o = 8;

  printf("character: %c\n", my_char);
  printf("string: %s\n", my_str);
  printf("long hex: %llX\n", my_long_hex);
  printf("multiple arguments: %c, %s, %llX, %d, %o\n", my_char, my_str, my_long_hex, my_n, my_o);
  printf("pointer: %p\n", &my_char);


  // Print out some ACPI information.
  // k_acpi_read();

  // Get the memory map.
  k_uefi_get_mem_map();

  // Terminate the boot services.
  int exited_uefi = k_uefi_exit();
  if (!exited_uefi)
  {
    fprintf(stddbg, "failed to exit UEFI boot services\n");
    fprintf(stderr, "failed to exit UEFI boot services\n");
  }
  else
  {
    fprintf(stddbg, "UEFI boot services have been terminated.\n");
  }
  
  // END UEFI boot services
  //==============================


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

  fprintf(stddbg, "Initialization complete.\n");

  // The main loop.
  for (;;);

  // We should never get here.
  return EFI_SUCCESS;
}
