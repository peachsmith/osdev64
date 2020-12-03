#ifndef JEP_SERIAL_H
#define JEP_SERIAL_H

// This is the interface for serial port I/O.
// The intended purpose of these functions is debugging in a virtual machine.

/**
 * Initializes a serial port.
 */
void k_serial_com1_init();

/**
 * Writes a character to a serial port.
 *
 * Params:
 *   c - a char to be written
 */
void k_serial_com1_putc(char c);

/**
 * Writes a string of characters to a serial port.
 *
 * Params:
 *   s - a pointer to a string of char data to be written
 */
void k_serial_com1_puts(char* s);

#endif
