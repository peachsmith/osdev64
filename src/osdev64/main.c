#include "osdev64/firmware.h"

#include "osdev64/bitmask.h"
#include "osdev64/instructor.h"
#include "osdev64/graphics.h"
#include "osdev64/serial.h"
#include "osdev64/console.h"
#include "osdev64/memory.h"
#include "osdev64/paging.h"
#include "osdev64/heap.h"

#include "osdev64/control.h"
#include "osdev64/cpuid.h"
#include "osdev64/msr.h"

#include "osdev64/descriptor.h"
#include "osdev64/interrupts.h"
#include "osdev64/acpi.h"
#include "osdev64/util.h"
#include "osdev64/mtrr.h"
#include "osdev64/pic.h"
#include "osdev64/pit.h"
#include "osdev64/apic.h"
#include "osdev64/task.h"
#include "osdev64/sync.h"
#include "osdev64/syscall.h"
#include "osdev64/ps2.h"
#include "osdev64/tty.h"

// temporary task demo for debugging task code
#include "osdev64/task_demo.h"

#include "osdev64/app_demo.h"

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
  k_firmware_init(image, systab);

  k_graphics_init();    // graphical output
  k_serial_com1_init(); // serial output
  k_memory_init();      // physical memory management
  k_heap_init();        // heap management
  k_console_init();     // text output
  k_acpi_init();        // ACPI tables
  k_sync_init();        // synchronization
  k_ps2_init();         // PS/2 keyboard emulation

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
      HANG();
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
    HANG();
  }

  // Get MAXPHYADDR from CPUID.80000008H:EAX[bits 7-0]
  maxphysaddr = k_cpuid_rax(0x80000008) & 0xFF;
  fprintf(stddbg, "[INFO] MAXPHYSADDR: %llu\n", maxphysaddr);

  // Check for MSR support.
  if (!(rdx_features & BM_5))
  {
    fprintf(stddbg, "[ERROR] MSRs are not supported\n");
    HANG();
  }

  // Check for MTRR support.
  if (!(rdx_features & BM_12))
  {
    fprintf(stddbg, "[ERROR] MTRRs are not supported\n");
    HANG();
  }

  // Check for PAT support.
  if (!(rdx_features & BM_16))
  {
    fprintf(stddbg, "[ERROR] PAT is not supported\n");
    HANG();
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
    fprintf(stddbg, "Local APIC ID: %u\n", lapic_id);

    // Get the local APIC version.
    uint32_t lapic_ver = k_lapic_get_version();
    fprintf(stddbg, "Local APIC Version: 0x%X\n", lapic_ver);

    // Get the local APIC max LVT.
    uint32_t lapic_maxlvt = k_lapic_get_maxlvt();
    fprintf(stddbg, "Local APIC Max LVT: %u\n", lapic_maxlvt);

    // Get the I/O APIC version.
    uint32_t ioapic_ver = k_ioapic_get_version();
    fprintf(stddbg, "I/O APIC Version: 0x%X\n", ioapic_ver);

    // Get the I/O APIC max redirects - 1.
    uint32_t ioapic_max = k_ioapic_get_max_redirect();
    fprintf(stddbg, "I/O APIC Max Redirects: %u\n", ioapic_max);


    // Disable the PIC since we're using APIC.
    k_pic_disable();

    // Enable the local APIC.
    k_lapic_enable();

    // Configure the IRQ redirection in the I/O APIC.
    k_ioapic_configure();

    fprintf(stddbg, "---------------------------------------\n");
  }
  else
  {
    fprintf(stddbg, "APIC unavailable\n");
  }

  // Initialize task management.
  k_task_init();

  // Install the syscall ISR at index 160
  k_install_isr(k_syscall_isr, 0xA0);

  // Enable interrupts.
  k_enable_interrupts();

  // Create the main task to represent this thread of execution.
  k_task* main_task = k_task_create(NULL);
  k_task_schedule(main_task);

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


  //==========================================
  // BEGIN physical memory demo
  //==========================================

  // // Print the physical RAM pool.
  // k_memory_print_pool();

  // // Allocate three separate regions of memory where
  // // each region is one page.
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

  //==========================================
  // END physical memory demo
  //==========================================



  //==========================================
  // BEGIN virtual address ledger
  //==========================================

  // fprintf(stddbg, "mapping a range of physical addreses into dynamic virtual address space\n");
  // // map some physical memory into dynamic virtual address space.
  // char* phys_dat = (char*)k_memory_alloc_pages(2);
  // char* virt_dat = (char*)k_paging_map_range(PTR_TO_N(phys_dat), PTR_TO_N(phys_dat) + 0x1000);

  // // Print the dynamic virtual address map ledger.
  // k_paging_print_ledger();

  // fprintf(stddbg, "mapping a second range\n");
  // // map some more physical memory into dynamic virtual address space.
  // char* phys_dat2 = (char*)k_memory_alloc_pages(2);
  // char* virt_dat2 = (char*)k_paging_map_range(PTR_TO_N(phys_dat2), PTR_TO_N(phys_dat2) + 0x1000);

  // // Print the dynamic virtual address map ledger.
  // k_paging_print_ledger();

  // fprintf(stddbg, "unmapping the first range\n");
  // k_paging_unmap_range(PTR_TO_N(virt_dat));

  // k_paging_print_ledger();

  // fprintf(stddbg, "mapping a third range\n");

  // // map yet more physical memory into dynamic virtual address space.
  // char* phys_dat3 = (char*)k_memory_alloc_pages(2);
  // char* virt_dat3 = (char*)k_paging_map_range(PTR_TO_N(phys_dat3), PTR_TO_N(phys_dat3) + 0x1000);

  // k_paging_print_ledger();

  // virt_dat3[0] = 'J';
  // virt_dat3[1] = 'E';
  // virt_dat3[2] = 'P';
  // virt_dat3[3] = '\0';

  // // virt_dat3 should be mapped to the physical location phys_dat3,
  // // so printing both should result in the same output.
  // fprintf(stddbg, "virt_dat3: %s\n", virt_dat3);
  // fprintf(stddbg, "phys_dat3: %s\n", phys_dat3);

  //==========================================
  // END virtual address ledger
  //==========================================


  // Test an exception handler.
  // k_cause_exception();

  // fprintf(stddbg, "RAM Pool:\n");
  // k_memory_print_pool();

  // fprintf(stddbg, "RAM Ledger:\n");
  // k_memory_print_ledger();

  // Print the MADT
  // k_acpi_print_madt();


  // Make a syscall to write a number to some place.
  k_syscall_face(0xF00D);

  // // Demonstrate mutual exclusion with locks.
  // k_task* mutex1 = k_task_create(mutex_demo_1);
  // k_task_schedule(mutex1);
  // while (mutex1->status != TASK_REMOVED);
  // k_task_destroy(mutex1);

  // // Demonstrate producer/consumer with semaphores.
  // k_task* sem1 = k_task_create(semaphore_demo_1);
  // k_task_schedule(sem1);
  // while (sem1->status != TASK_REMOVED);
  // k_task_destroy(sem1);

  // END demo code
  //==============================


  fprintf(stddbg, "[INFO] Initialization complete.\n");

  // k_heap_print();

  // k_byte* h1 = (k_byte*)k_heap_alloc(16);
  // if (h1 == NULL)
  // {
  //   fprintf(stddbg, "failed to allocate first chunk of memory\n");
  // }
  // k_heap_print();

  // k_byte* h2 = (k_byte*)k_heap_alloc(24);
  // if (h2 == NULL)
  // {
  //   fprintf(stddbg, "failed to allocate second chunk of memory\n");
  // }
  // k_heap_print();

  // k_byte* h3 = (k_byte*)k_heap_alloc(24);
  // if (h3 == NULL)
  // {
  //   fprintf(stddbg, "failed to allocate third chunk of memory\n");
  // }
  // k_heap_print();

  // k_heap_free(h2);
  // k_heap_print();

  // k_heap_free(h1);
  // k_heap_print();

  // h1 = (k_byte*)k_heap_alloc(8);
  // if (h1 == NULL)
  // {
  //   fprintf(stddbg, "failed to allocate first chunk of memory\n");
  // }
  // k_heap_print();

  // Start the kernel terminal emulator.
  k_tty_init();

  FILE* shell_out = (FILE*)k_task_get_io_buffer(__FILE_NO_STDOUT);

  int count = 0;

  k_acpi_read_mcfg();

  // The main loop.
  for (;;)
  {
    // Do stuff
    if (count < 10)
    {
      k_pit_wait(120);
      // printf("Hello, World!\n");
      printf("[DEBUG] count: %d\n", count++);
      // printf(shell_out, "[DEBUG] count: %d\n", count++);
    }

    // fprintf(
    //   stddbg,
    //   "LEFT: %d, UP: %d, RIGHT: %d\n",
    //   kstates[PS2_SC_LEFT] ? 1 : 0,
    //   kstates[PS2_SC_UP] ? 1 : 0,
    //   kstates[PS2_SC_RIGHT] ? 1 : 0
    // );
    // fprintf(stddbg, "This is the main loop\n");
  }

  // We should never get here.
  return EFI_SUCCESS;
}
