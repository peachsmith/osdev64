#include "osdev64/firmware.h"

#include "osdev64/bitmask.h"
#include "osdev64/instructor.h"
#include "osdev64/control.h"
#include "osdev64/cpuid.h"
#include "osdev64/msr.h"
//#include "osdev64/uefi.h"
#include "osdev64/graphics.h"
#include "osdev64/serial.h"
#include "osdev64/console.h"
#include "osdev64/memory.h"
#include "osdev64/paging.h"
#include "osdev64/descriptor.h"
#include "osdev64/acpi.h"
#include "osdev64/util.h"
#include "osdev64/mtrr.h"
#include "osdev64/pic.h"
#include "osdev64/pit.h"
#include "osdev64/apic.h"
#include "osdev64/task.h"

#include "klibc/stdio.h"


void task_a_action()
{
  for (int i = 0; i < 10; i++)
  {
    k_apic_wait(60);
    fprintf(stddbg, "This is task A\n");
  }
}

void task_b_action()
{
  for (;;)
  {
    k_apic_wait(120);
    fprintf(stddbg, "This is task B\n");
  }
}


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
  k_firmware_init(image, systab);

  k_graphics_init();    // graphical output
  k_serial_com1_init(); // serial output
  k_console_init();     // text output
  k_memory_init();      // memory management
  k_acpi_init();        // ACPI tables

  fprintf(stddbg, "[INFO] graphics, serial, console, and memory have been initialized.\n");

  // CPU information
  k_regn cr0 = k_get_cr0();
  k_regn cr4 = k_get_cr4();
  k_regn rflags = k_get_rflags();
  k_regn max_e = k_cpuid_rax(0x80000000);
  k_regn maxphysaddr;
  k_regn rdx_features = k_cpuid_rdx(1);
  char vendor[13];

  // Check for CPUID.
  if (!(rflags & BM_21))
  {
    k_set_rflags(rflags | BM_21);
    rflags = k_get_rflags();
    if (rflags & BM_21)
    {
      fprintf(stddbg, "[INFO] CPUID enabled\n");
    }
    else
    {
      fprintf(stddbg, "[ERROR] CPUID unavailable\n");
      for (;;);
    }
  }

  // Vendor Identification String
  k_cpuid_vendor(vendor);
  vendor[12] = '\0';
  fprintf(stddbg, "[INFO] Vendor: %s\n", vendor);

  // Check for max extension.
  if (max_e >= 0x80000008)
  {
    fprintf(stddbg, "[INFO] Max Extension: %llX\n", max_e);
  }
  else
  {
    fprintf(stddbg, "[ERROR] Max Extension too low: %llu\n", max_e);
    for (;;);
  }

  // Get MAXPHYADDR from CPUID.80000008H:EAX[bits 7-0]
  maxphysaddr = k_cpuid_rax(0x80000008) & 0xFF;
  fprintf(stddbg, "[INFO] MAXPHYSADDR: %llu\n", maxphysaddr);

  // Check for MSR support.
  if (!(rdx_features & BM_5))
  {
    fprintf(stddbg, "[ERROR] MSRs are not supported\n");
    for (;;);
  }

  // Check for MTRR support.
  if (!(rdx_features & BM_12))
  {
    fprintf(stddbg, "[ERROR] MTRRs are not supported\n");
    for (;;);
  }

  // Check for PAT support.
  if (!(rdx_features & BM_16))
  {
    fprintf(stddbg, "[ERROR] PAT is not supported\n");
    for (;;);
  }

  // Write some bits from CR0 and CR4
  fprintf(stddbg, "[DEBUG] CR0.PE:    %c\n", (cr0 & BM_0) ? 'Y' : 'N');
  fprintf(stddbg, "[DEBUG] CR0.NW:    %c\n", (cr0 & BM_29) ? 'Y' : 'N');
  fprintf(stddbg, "[DEBUG] CR0.CD:    %c\n", (cr0 & BM_30) ? 'Y' : 'N');
  fprintf(stddbg, "[DEBUG] CR0.PG:    %c\n", (cr0 & BM_31) ? 'Y' : 'N');
  fprintf(stddbg, "[DEBUG] CR4.PSE:   %c\n", (cr4 & BM_4) ? 'Y' : 'N');
  fprintf(stddbg, "[DEBUG] CR4.PAE:   %c\n", (cr4 & BM_5) ? 'Y' : 'N');
  fprintf(stddbg, "[DEBUG] CR4.PGE:   %c\n", (cr4 & BM_7) ? 'Y' : 'N');
  fprintf(stddbg, "[DEBUG] CR4.PCIDE: %c\n", (cr4 & BM_17) ? 'Y' : 'N');
  fprintf(stddbg, "[DEBUG] CR4.PKE:   %c\n", (cr4 & BM_22) ? 'Y' : 'N');

  // END Stage 1 initialization
  //==============================


  //==============================
  // BEGIN Stage 2 initialization

  // Disable interrupts.
  k_disable_interrupts();

  // Build the initial GDT.
  k_gdt_init();

  // Build the initial IDT.
  k_idt_init();

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

  // Create a new virtual address space.
  k_paging_init();

  // Map the framebuffer to some high virtual address.
  k_graphics_map_framebuffer();

  // Initialize the PIC.
  k_pic_init();

  // Initialize the PIT to start generating timer IRQs.
  k_pit_init();

  // Initialize the APIC interface.
  k_apic_init();

  // If APICs are available, disable the PIC and
  // configure the APICs to handle interrupts.
  if (k_apic_available())
  {
    // Get the local APIC ID.
    uint32_t lapic_id = k_lapic_get_id();
    printf("Local APIC ID: %u\n", lapic_id);

    // Get the local APIC version.
    uint32_t lapic_ver = k_lapic_get_version();
    printf("Local APIC Version: 0x%X\n", lapic_ver);

    // Get the local APIC max LVT.
    uint32_t lapic_maxlvt = k_lapic_get_maxlvt();
    printf("Local APIC Max LVT: %u\n", lapic_maxlvt);

    // Get the I/O APIC version.
    uint32_t ioapic_ver = k_ioapic_get_version();
    printf("I/O APIC Version: 0x%X\n", ioapic_ver);

    // Get the I/O APIC max redirects - 1.
    uint32_t ioapic_max = k_ioapic_get_max_redirect();
    printf("I/O APIC Max Redirects: %u\n", ioapic_max);


    // Disable the PIC since we're using APIC.
    k_pic_disable();

    // Enable the local APIC.
    k_lapic_enable();

    // Configure the IRQ redirection in the I/O APIC.
    k_ioapic_configure();

    printf("---------------------------------------\n");
  }
  else
  {
    printf("APIC unavailable\n");
  }

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

  // Print the physical RAM pool.
  // k_memory_print_pool();

  // Allocate three separate regions of memory where
  // each region is one page.
  // fprintf(stddbg, "creating 3 separate 1 page reservations\n");
  // char* my_ram = (char*)k_memory_alloc_pages(1);
  // char* my_ram2 = (char*)k_memory_alloc_pages(1);
  // char* my_ram3 = (char*)k_memory_alloc_pages(1);
  // char* my_ram4 = NULL;
  // k_memory_print_ledger();

  // // Free the second region of memory.
  // fprintf(stddbg, "freeing 1 page\n");
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

  // fprintf(stddbg, "RAM Pool:\n");
  // k_memory_print_pool();

  // fprintf(stddbg, "RAM Ledger:\n");
  // k_memory_print_ledger();

  // Print the MADT
  // k_acpi_print_madt();

  fprintf(stddbg, "[INFO] Initialization complete.\n");

  fprintf(stddbg, "mapping a range of physical addreses into dynamic virtual address space\n");
  // map some physical memory into dynamic virtual address space.
  char* phys_dat = (char*)k_memory_alloc_pages(2);
  char* virt_dat = (char*)k_paging_map_range(PTR_TO_N(phys_dat), PTR_TO_N(phys_dat) + 0x1000);

  // Print the dynamic virtual address map ledger.
  k_paging_print_ledger();

  fprintf(stddbg, "mapping a second range\n");
  // map some more physical memory into dynamic virtual address space.
  char* phys_dat2 = (char*)k_memory_alloc_pages(2);
  char* virt_dat2 = (char*)k_paging_map_range(PTR_TO_N(phys_dat2), PTR_TO_N(phys_dat2) + 0x1000);

  // Print the dynamic virtual address map ledger.
  k_paging_print_ledger();

  fprintf(stddbg, "unmapping the first range\n");
  k_paging_unmap_range(PTR_TO_N(virt_dat));

  k_paging_print_ledger();

  fprintf(stddbg, "mapping a third range\n");

  // map yet more physical memory into dynamic virtual address space.
  char* phys_dat3 = (char*)k_memory_alloc_pages(2);
  char* virt_dat3 = (char*)k_paging_map_range(PTR_TO_N(phys_dat3), PTR_TO_N(phys_dat3) + 0x1000);

  k_paging_print_ledger();

  // k_task* main_task = k_task_create(NULL);
  // k_task* task_a = k_task_create(task_a_action);
  // k_task* task_b = k_task_create(task_b_action);

  // k_task_schedule(main_task);
  // k_task_schedule(task_a);
  // k_task_schedule(task_b);

  int should_stop_b = 0;

  // The main loop.
  for (;;)
  {
    // k_apic_wait(240);
    // fprintf(stddbg, "This is the main task.\n");

    // if (task_a != NULL && task_a->status == TASK_REMOVED)
    // {
    //   fprintf(stddbg, "destroying task a\n");
    //   k_task_destroy(task_a);
    //   task_a = NULL;
    // }

    // if (task_b != NULL && task_b->status == TASK_REMOVED)
    // {
    //   fprintf(stddbg, "destroying task b\n");
    //   k_task_destroy(task_b);
    //   task_b = NULL;
    // }

    // if (should_stop_b < 10)
    // {
    //   should_stop_b++;
    //   if (should_stop_b == 10)
    //   {
    //     k_task_stop(task_b);
    //   }
    // }
  }

  // We should never get here.
  return EFI_SUCCESS;
}
