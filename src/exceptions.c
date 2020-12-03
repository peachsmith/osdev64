// This file contains functions for handling exceptions.
// These functions are called from ISRs.

#include "core.h"
#include "serial.h"

/**
 * Handles a divide error, which triggers interrupt 0.
 */
void div0_handler()
{
  k_serial_com1_puts("divide error\n");
  for (;;);
}

/**
 * Handles a general protection fault, which trigger interrupt 13.
 */
void gp_fault_handler()
{
  k_serial_com1_puts("general protection fault\n");
  for (;;);
}

/**
 * Handles a page fault, which trigger interrupt 14.
 */
void page_fault_handler()
{
  k_serial_com1_puts("page fault\n");
  for (;;);
}

/**
 * Generic exception handler
 */
void generic_handler()
{
  k_serial_com1_puts("generic fault\n");
  for (;;);
}

void my_irq()
{
  k_serial_com1_puts("Hello from an IRQ handler!\n");
}
