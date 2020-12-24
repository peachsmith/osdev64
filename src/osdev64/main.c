#include "osdev64/instructor.h"
#include "osdev64/uefi.h"
#include "osdev64/graphics.h"
#include "osdev64/serial.h"
#include "osdev64/console.h"
#include "osdev64/memory.h"
#include "osdev64/paging.h"
#include "osdev64/descriptor.h"
#include "osdev64/acpi.h"

#include "klibc/stdio.h"


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


  // Write some text to standard output and debug output.
  fputs("console output with fputs\n", stdout);
  fputs("serial output with fputs\n", stddbg);

  // // Write some formatted text to standard output.
  // char my_char = 'J';
  // char* my_str = "bagel";
  // uint64_t my_long_hex = 0xDEAD0000BEEF0000;
  // int my_n = -17;
  // int my_o = 8;

  // printf("character: %c\n", my_char);
  // printf("string: %s\n", my_str);
  // printf("long hex: %llX\n", my_long_hex);
  // printf("multiple arguments: %c, %s, %llX, %d, %o\n", my_char, my_str, my_long_hex, my_n, my_o);
  // printf("pointer: %p\n", &my_char);

  // // Print the physical RAM pool.
  // k_memory_print_pool();

  // // Allocate three separate regions of memory where
  // // each region is one page.
  // char* my_ram = (char*)k_memory_alloc_pages(1);
  // char* my_ram2 = (char*)k_memory_alloc_pages(1);
  // char* my_ram3 = (char*)k_memory_alloc_pages(1);
  // char* my_ram4 = NULL;
  // k_memory_print_ledger();

  // // Free the second region of memory.
  // fprintf(stddbg, "freeing my_ram2\n");
  // k_memory_free_pages((void*)my_ram2);
  // k_memory_print_ledger();

  // // Allocate three pages for the second memory region.
  // fprintf(stddbg, "reserving 3 pages\n");
  // my_ram2 = (char*)k_memory_alloc_pages(3);
  // k_memory_print_ledger();

  // // Allocate one more page. 
  // fprintf(stddbg, "reserving 1 page\n");
  // my_ram4 = (char*)k_memory_alloc_pages(1);
  // k_memory_print_ledger();

  // END demo code
  //==============================


  //==============================
  // BEGIN Stage 2 initialization

  // Disable interrupts.
  k_disable_interrupts();


  // Read the control registers and RFLAGS
  uint64_t cr0 = k_get_cr0();
  uint64_t cr4 = k_get_cr4();
  uint64_t rflags = k_get_rflags();
  uint64_t cr_mask = 1;

  // CR0 bits
  // PE - protected mode enabled
  // EM - x87 floating point emulation
  // PG - paging enabled
  printf("CR0.PE: %c\n", (cr0 & CR0_PE) ? 'Y' : 'N');
  printf("CR0.EM: %c\n", (cr0 & CR0_EM) ? 'Y' : 'N');
  printf("CR0.PG: %c\n", (cr0 & CR0_PG) ? 'Y' : 'N');

  // CR4 bits
  // PAE - physical address extension
  // PCIDE - process context identifiers enabled
  // PKE - protection key enabled
  printf("CR4.PAE: %c\n", (cr4 & CR4_PAE) ? 'Y' : 'N');
  printf("CR4.PCIDE: %c\n", (cr4 & CR4_PCIDE) ? 'Y' : 'N');
  printf("CR4.SMEP: %c\n", (cr4 & CR4_SMEP) ? 'Y' : 'N');
  printf("CR4.SMAP: %c\n", (cr4 & CR4_SMAP) ? 'Y' : 'N');
  printf("CR4.PKE: %c\n", (cr4 & CR4_PKE) ? 'Y' : 'N');

  // Clear CR4.PCIDE if it's set.
  // I don't want to mess around with
  if (cr4 & CR4_PCIDE)
  {
    cr4 &= ~CR4_PCIDE;
    k_set_cr4(cr4);
  }

  // Clear CR4.PKE if it's set.
  // I can't be bothered to deal with protection keys in
  // my paging structures.
  if (cr4 & CR4_PKE)
  {
    cr4 &= ~CR4_PKE;
    k_set_cr4(cr4);
  }

  // RFLAGS bits
  // Set bit 21 of RFLAGS to confirm that CPUID is available
  if (!(rflags & RFLAGS_CPUID))
  {
    rflags |= RFLAGS_CPUID;
    k_set_rflags(rflags);

    rflags = k_get_rflags();
  }
  printf("CPUID: %c\n", (rflags & RFLAGS_CPUID) ? 'Y' : 'N');

  

  // Load the GDT.
  k_load_gdt();

  // Load the IDT.
  k_load_idt();

  // Replace UEFI's paging with our own.
  // k_paging_init();


  // Enable interrupts.
  k_enable_interrupts();

  // END Stage 2 initialization
  //==============================


  // Test an exception handler.
  // k_cause_exception();

  fprintf(stddbg, "\n\nInitialization complete.\n");

  // The main loop.
  for (;;);

  // We should never get here.
  return EFI_SUCCESS;
}
