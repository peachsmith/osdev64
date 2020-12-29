#include "osdev64/bitmask.h"
#include "osdev64/instructor.h"
#include "osdev64/control.h"
#include "osdev64/cpuid.h"
#include "osdev64/msr.h"
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

  k_graphics_init();    // graphical output
  k_serial_com1_init(); // serial output
  k_console_init();     // text output
  k_memory_init();      // memory management
  // TODO: ACPI

  // CPU information
  uint64_t cr0 = k_get_cr0();
  uint64_t cr4 = k_get_cr4();
  uint64_t rflags = k_get_rflags();
  uint64_t max_e = k_cpuid_rax(0x80000000);
  uint64_t maxphysaddr;
  uint64_t rdx_features = k_cpuid_rdx(1);
  char vendor[13];

  // Check for CPUID.
  if (rflags & BM_21)
  {
    fprintf(stddbg, "[CPUID] available\n");
  }
  else
  {
    k_set_rflags(rflags | BM_21);
    rflags = k_get_rflags();
    if (rflags & BM_21)
    {
      fprintf(stddbg, "[CPUID] enabled\n");
    }
    else
    {
      fprintf(stddbg, "[CPUID] unavailable\n");
      for (;;);
    }
  }

  // Vendor Identification String
  k_cpuid_vendor(vendor);
  vendor[12] = '\0';
  fprintf(stddbg, "[CPUID] Vendor: %s\n", vendor);

  // Check for max extension.
  if (max_e >= 0x80000008)
  {
    fprintf(stddbg, "[CPUID] Max Extension: %llX\n", max_e);
  }
  else
  {
    fprintf(stddbg, "[CPUID] Max Extension too low: %llu\n", max_e);
    for (;;);
  }

  // Get MAXPHYADDR from CPUID.80000008H:EAX[bits 7-0]
  maxphysaddr = k_cpuid_rax(0x80000008) & 0xFF;
  fprintf(stddbg, "[CPUID] MAXPHYSADDR: %llu\n", maxphysaddr);

  // Check for MSR support.
  if (!(rdx_features & BM_5))
  {
    fprintf(stddbg, "[CPUID] MSRs are not supported\n");
    for (;;);
  }

  // Check for MTRR support.
  if (!(rdx_features & BM_12))
  {
    fprintf(stddbg, "[CPUID] MTRRs are not supported\n");
    for (;;);
  }

  // Check for PAT support.
  if (!(rdx_features & BM_16))
  {
    fprintf(stddbg, "[CPUID] The PAT is not supported\n");
    for (;;);
  }

  // Write some bits from CR0 and CR4
  fprintf(stddbg, "[CR0] PE:    %c\n", (cr0 & BM_0) ? 'Y' : 'N');
  fprintf(stddbg, "[CR0] NW:    %c\n", (cr0 & BM_29) ? 'Y' : 'N');
  fprintf(stddbg, "[CR0] CD:    %c\n", (cr0 & BM_30) ? 'Y' : 'N');
  fprintf(stddbg, "[CR0] PG:    %c\n", (cr0 & BM_31) ? 'Y' : 'N');
  fprintf(stddbg, "[CR4] PSE:   %c\n", (cr4 & BM_4) ? 'Y' : 'N');
  fprintf(stddbg, "[CR4] PAE:   %c\n", (cr4 & BM_5) ? 'Y' : 'N');
  fprintf(stddbg, "[CR4] PGE:   %c\n", (cr4 & BM_7) ? 'Y' : 'N');
  fprintf(stddbg, "[CR4] PCIDE: %c\n", (cr4 & BM_17) ? 'Y' : 'N');
  fprintf(stddbg, "[CR4] PKE:   %c\n", (cr4 & BM_22) ? 'Y' : 'N');

  // Terminate UEFI boot services.
  if (!k_uefi_exit())
  {
    fprintf(stddbg, "[UEFI] Failed to exit UEFI boot services.\n");
    for (;;);
  }

  // END Stage 1 initialization
  //==============================


  //==============================
  // BEGIN Stage 2 initialization

  // Disable interrupts.
  k_disable_interrupts();

  // Load the GDT.
  k_load_gdt();

  // Load the IDT.
  k_load_idt();

  // Clear CR4.PCIDE
  if (cr4 & CR4_PCIDE)
  {
    cr4 &= ~CR4_PCIDE;
    k_set_cr4(cr4);
  }

  // Clear CR4.PKE
  if (cr4 & CR4_PKE)
  {
    cr4 &= ~CR4_PKE;
    k_set_cr4(cr4);
  }

  // Replace UEFI's paging with our own.
  k_paging_init();

  // Enable interrupts.
  k_enable_interrupts();


  // END Stage 2 initialization
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


  // Test an exception handler.
  // k_cause_exception();

  fprintf(stddbg, "\n\nInitialization complete.\n");

  // The main loop.
  for (;;);

  // We should never get here.
  return EFI_SUCCESS;
}
