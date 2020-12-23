#include "serial.h"

#include <stdint.h>

// port I/O functions
void k_outb(uint16_t port, uint8_t val);
uint8_t k_inb(uint16_t port);

#define COM1 0x03F8

// check if the transmission line is available
static uint8_t is_transmit_empty(uint16_t com)
{
  return k_inb(com + 5) & 0x20;
}

void k_serial_com1_init()
{
  k_outb(COM1 + 1, 0x00); // Disable all interrupts
  k_outb(COM1 + 3, 0x80); // Enable DLAB (set baud rate divisor)
  k_outb(COM1 + 0, 0x03); // Set divisor to 3 (lo byte) 38400 baud
  k_outb(COM1 + 1, 0x00); //                  (hi byte)
  k_outb(COM1 + 3, 0x03); // 8 bits, no parity, one stop bit
  k_outb(COM1 + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
  k_outb(COM1 + 4, 0x0B); // IRQs enabled, RTS/DSR set
}


void k_serial_com1_putc(char c)
{
  while (is_transmit_empty(COM1) == 0);

  k_outb(COM1, (uint8_t)c);
}


void k_serial_com1_puts(const char* s)
{
  while (*s != '\0')
  {
    k_serial_com1_putc(*s);
    s++;
  }
}
