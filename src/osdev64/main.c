#include "osdev64/uefi.h"
#include "osdev64/graphics.h"
#include "osdev64/serial.h"
#include "osdev64/console.h"
#include "osdev64/memory.h"
#include "osdev64/descriptor.h"
#include "osdev64/acpi.h"

#include "klibc/stdio.h"


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
  // BEGIN Stage 1 initialization

  // Initialize UEFI boot services.
  k_uefi_init(image, systab);

  k_graphics_init();          // graphical output
  k_serial_com1_init();       // serial output
  k_console_init();           // text output
  k_memory_init();            // memory management
  // TODO: ACPI

  // Terminate UEFI boot services.
  k_uefi_exit();

  // END Stage 1 initialization
  //==============================


  //==============================
  // BEGIN demo code

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


  // Write a "Hello, World" message to the console and to serial output.
  fputs("console output with fputs\n", stdout);
  fputs("serial output with fputs\n", stddbg);

  // Write some formatted standard output.
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

  // END demo code
  //==============================


  //==============================
  // BEGIN Stage 2 initialization

  // Disable interrupts.
  k_disable_interrupts();

  // Load the GDT.
  k_load_gdt();

  // Load the IDT.
  k_load_idt();

  // Enable interrupts.
  k_enable_interrupts();

  // END Stage 2 initialization
  //==============================


  // Test an exception handler.
  // k_cause_exception();

  fprintf(stddbg, "Initialization complete.\n");

  // The main loop.
  for (;;);

  // We should never get here.
  return EFI_SUCCESS;
}
