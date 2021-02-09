#include "osdev64/pit.h"
#include "osdev64/instructor.h"

#include <stdint.h>

uint64_t g_pit_ticks = 0;

void k_pit_init()
{
  // This implementation is based on the example provided in James Molloy's
  // kernel development tutorial at
  // http://www.jamesmolloy.co.uk/tutorial_html/


  // The value we send to the PIT is the value to divide it's input clock
  // (1193180 Hz) by, to get our required frequency. Important to note is
  // that the divisor must be small enough to fit into 16-bits.
  uint32_t divisor = 1193180 / 60;

  // Send the command byte.
  k_outb(0x43, 0x36);

  // Divisor has to be sent byte-wise, so split here into upper/lower bytes.
  uint8_t l = (uint8_t)(divisor & 0xFF);
  uint8_t h = (uint8_t)((divisor >> 8) & 0xFF);

  // Send the frequency divisor.
  k_outb(0x40, l);
  k_outb(0x40, h);
}

void k_pit_wait(uint64_t n)
{
  uint64_t current = g_pit_ticks;

  while (g_pit_ticks - current < n);
}