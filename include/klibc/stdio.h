#ifndef JEP_STDIO_H
#define JEP_STDIO_H

#include "osdev64/file.h"
#include <stdarg.h>

typedef struct k_iobuf {
  void* info;
}FILE;

FILE* k_get_iobuf(int);

// #define __FILE_NO_STDIN  1
// #define __FILE_NO_STDOUT 2
// #define __FILE_NO_STDERR 3
// #define __FILE_NO_STDDBG 4

#define stdin k_get_iobuf(__FILE_NO_STDIN)
#define stdout k_get_iobuf(__FILE_NO_STDOUT)
#define stderr k_get_iobuf(__FILE_NO_STDERR)
#define stddbg k_get_iobuf(__FILE_NO_STDDBG)

/**
 * Writes a character to an output stream.
 * This is the same as fputc, but it may be implemented as a macro.
 *
 * Params:
 *   int - the character to be written
 *   FILE* - the output stream to receive the character
 */
int putc(int, FILE*);

/**
 * Writes a character to an output stream.
 *
 * Params:
 *   int - the character to be written
 *   FILE* - the output stream to receive the character
 */
int fputc(int, FILE*);

/**
 * Writes a NUL-terminated string of characters to standard output.
 * The NUL character '\0' is not writtern.
 *
 * Params:
 *   const char* - a pointer to a NUL-terminated string of characters
 *
 * Returns:
 *   int - the number of characters written, or -1 on failure.
 */
int puts(const char*);

/**
 * Writes a NUL-terminated string of characters to an output stream.
 * The NUL character '\0' is not writtern.
 *
 * Params:
 *   const char* - a pointer to a NUL-terminated string of characters
 *   FILE* - an output stream
 *
 * Returns:
 *   int - the number of characters written, or -1 on failure.
 */
int fputs(const char*, FILE*);

/**
 * Writes a formatted string to standard output.
 *
 * Params:
 *   const char* - a NUL-terminated string of characters
 *   ... - a variable length list of arguments to be formatted
 */
int printf(const char*, ...);

/**
 * Writes a formatted string to an output stream.
 *
 * Params:
 *   FILE* - an output stream
 *   const char* - a NUL-terminated string of characters
 *   ... - a variable length list of arguments to be formatted
 */
int fprintf(FILE*, const char*, ...);

/**
 * Writes a formatted string to an output stream.
 * Since this function uses va_list, calls to this function should be
 * preceeded by va_start and followed by va_end.
 *
 * Params:
 *   FILE* - an output stream
 *   const char* - a NUL-terminated string of characters
 *   va_list - a variable length list of arguments to be formatted
 */
int vfprintf(FILE*, const char*, va_list);

#endif