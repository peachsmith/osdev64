// This file contains functions for handling exceptions.
// These functions are called from ISRs.

#include "osdev64/axiom.h"

#include "klibc/stdio.h"

extern uint64_t g_endpoint;

/**
 * Handles a divide error, which triggers interrupt 0.
 */
void div0_handler()
{
  fprintf(stddbg, "divide error\n");
  // fprintf(stderr, "divide error\n");
  for (;;);
}

/**
 * Handles a general protection fault, which trigger interrupt 13.
 */
void gp_fault_handler()
{
  fprintf(stddbg, "general protection fault\n");
  // fprintf(stderr, "general protection fault\n");
  for (;;);
}

/**
 * Handles a page fault, which trigger interrupt 14.
 */
void page_fault_handler()
{
  fprintf(stddbg, "page fault. last endpoint: %llX\n", g_endpoint);
  // fprintf(stderr, "page fault\n");
  for (;;);
}

void my_irq()
{
  fprintf(stddbg, "Hello from an IRQ handler!\n");
}
