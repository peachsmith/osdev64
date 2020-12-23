#ifndef JEP_CONSOLE_H
#define JEP_CONSOLE_H


/**
 * Initializes the text console.
 * This must be called once before any of the other console functions
 * are called.
 */
void k_console_init();


/**
 * Writes a single character to the console.
 *
 * Params:
 *   char* - a NUL-terminated string of characters.
 */
void k_console_putc(char);


/**
 * Writes a NUL-terminated character string to the console.
 * This function assumes that the string ends with the '\0' character.
 *
 * Params:
 *   char* - a NUL-terminated string of characters.
 */
void k_console_puts(char*);

#endif