#include "klibc/stdio.h"

#include <stddef.h>

struct k_finfo {
  int type;
  size_t pos;
};

// standard I/O streams.
struct k_finfo k_in_info = { __FILE_NO_STDIN, 0 };
struct k_finfo k_out_info = { __FILE_NO_STDOUT, 0 };
struct k_finfo k_err_info = { __FILE_NO_STDERR, 0 };

FILE k_stdin = { (void*)&k_in_info };
FILE k_stdout = { (void*)&k_out_info };
FILE k_stderr = { (void*)&k_err_info };

// custom I/O stream for debugging.
struct k_finfo k_dbg_info = { __FILE_NO_STDDBG, 0 };
FILE k_stddbg = { (void*)&k_dbg_info };

FILE* k_get_iobuf(int id)
{
  switch (id)
  {
  case 1: return &k_stdin;
  case 2: return &k_stdout;
  case 3: return &k_stderr;
  case 4: return &k_stddbg;
  default: return NULL;
  }
}


// external functions for writing characters to some form of output.
void k_console_putc(char);
void k_serial_com1_putc(char);
void k_serial_com1_puts(char*);


int putc(int c, FILE* stream)
{
  return fputc(c, stream);
}

int fputc(int c, FILE* stream)
{
  struct k_finfo* inf = (struct k_finfo*)(stream->info);

  switch (inf->type)
  {
  case __FILE_NO_STDOUT:
  case __FILE_NO_STDERR:
    k_console_putc(c & 0xFF);
    return 1;

  case __FILE_NO_STDDBG:
    k_serial_com1_putc(c & 0xFF);
    return 1;

  default:
    return 0;
  }
}

// Implementation of
//   memcpy
//   memmove
//   strlen
#include "string.c"

// Implementation of
//   fputs
//   puts
#include "puts.c"

// Implementation of
//   printf
//   fprintf
//   vfprintf
#include "printf.c"
