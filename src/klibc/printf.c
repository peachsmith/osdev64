#ifndef JEP_PRINTF_C
#define JEP_PRINTF_C

/**
 * Implementations of the following functions:
 *   printf
 *   fprintf
 *   vfprintf
 *
 * Semicolons are place between some macro definitions as a workaround
 * to prevent Visual Studio code from indenting doc comments excessively.
 */

;

#include "klibc/stdio.h"
#include "klibc/string.h"


#include <stdint.h>
#include <stddef.h>


#include "format.c"


int printf(const char* format, ...)
{
  va_list arg;
  int res;

  va_start(arg, format);
  res = vfprintf(stdout, format, arg);
  va_end(arg);

  return res;
}


int fprintf(FILE* stream, const char* format, ...)
{
  va_list arg;
  int res;

  va_start(arg, format);
  res = vfprintf(stream, format, arg);
  va_end(arg);

  return res;
}


int vfprintf(FILE* stream, const char* format, va_list args)
{
  size_t i;
  size_t j;
  char* end;     // position pointer
  char buf[128]; // string conversion buffer
  size_t len;    // string len
  int err;       // error flag

  // Values to be printed
  int n;          // integer
  int64_t n64;    // long integer
  unsigned int u; // unsigned integer
  uint32_t u8;    // 8-bit unsigned integer
  uint32_t u16;   // 16-bit unsigned integer
  uint32_t u32;   // 32-bit unsigned integer
  uint64_t u64;   // unsigned long integer
  uintptr_t p;    // pointer
  double d;       // floating point number


  i = j = 0;
  err = 0;

  while (*format != '\0' && !err)
  {
    if (*format == '%')
    {
      format++;
      ftag t = parse_format(format, &end);
      format = end;

      // Get the width from the argument list.
      if (t.flags & FMT_WIDTH)
      {
        t.width = va_arg(args, size_t);
      }

      // Get the precision from the argument list.
      if (t.flags & FMT_PREC)
      {
        k_serial_com1_puts("we should NOT be here getting the precision\n");
        t.prec = va_arg(args, size_t);
      }

      if (t.spec == SPEC_per)
      {
        // The '%' character
        fputc('%', stream);
      }
      else if (t.spec == SPEC_c)
      {
        // All printable characters except '%'.
        char c = va_arg(args, int);
        fputc(c, stream);
      }
      else if (t.spec == SPEC_s)
      {
        // strings
        char* s = va_arg(args, char*);
        size_t len = strlen(s);

        // Pad with spaces to the left if right-justified.
        if (t.width > 0 && !(t.flags & FMT_LEFT))
        {
          if (len < t.width)
          {
            for (int pad = 0; pad < t.width - len; pad++)
            {
              fputc(' ', stream);
            }
          }
        }

        // Write the string.
        for (i = 0; i < len; i++)
        {
          fputc(s[i], stream);
        }

        // Pad with spaces to the right if left-justified.
        if (t.width > 0 && (t.flags & FMT_LEFT))
        {
          if (len < t.width)
          {
            for (int pad = 0; pad < t.width - len; pad++)
            {
              fputc(' ', stream);
            }
          }
        }

      }
      else if (t.spec == SPEC_d || t.spec == SPEC_i)
      {
        // signed decimal integers
        if (t.len & LEN_ll)
        {
          n64 = va_arg(args, int64_t);
          len = int64_to_buffer(n64, buf, t);
        }
        else
        {
          n = va_arg(args, int);
          len = int_to_buffer(n, buf, t);
        }
        print_num(stream, buf, len, t);
      }
      else if (t.spec == SPEC_X || t.spec == SPEC_x || t.spec == SPEC_u || t.spec == SPEC_o)
      {
        // unsigned integers
        if (t.len & LEN_ll)
        {
          n64 = va_arg(args, int64_t);
          len = int64_to_buffer(n64, buf, t);
        }
        else
        {
          n = va_arg(args, int);
          len = int_to_buffer(n, buf, t);
        }
        print_num(stream, buf, len, t);
      }
      else if (t.spec == SPEC_p)
      {
        // pointers
        p = va_arg(args, uintptr_t);
        len = uptr_to_buffer(p, buf, 1);

        print_num(stream, buf, len, t);
      }
      else if (t.spec == SPEC_b)
      {
        // binary
        // This format specifier is NOT standard.
        u64 = 0;

        // The preceision should be either 8, 16, 32, or 64.
        switch (t.prec)
        {
        case 8:
          u8 = (uint8_t)va_arg(args, int);
          len = bin_to_buffer((u64 | u8), buf, 8);
          print_num(stream, buf, len, t);
          break;

        case 16:
          u16 = (uint16_t)va_arg(args, int);
          len = bin_to_buffer((u64 | u16), buf, 16);
          print_num(stream, buf, len, t);
          break;

        case 32:
          u32 = va_arg(args, uint32_t);
          len = bin_to_buffer((u64 | u32), buf, 32);
          print_num(stream, buf, len, t);
          break;

        case 64:
          u64 = va_arg(args, uint64_t);
          len = bin_to_buffer(u64, buf, 64);
          print_num(stream, buf, len, t);
          break;

        default:
          break;
        }
      }
      else if (t.spec == SPEC_f)
      {
        // floating point numbers
        // not implemented
      }
      else if (t.spec == SPEC_E || t.spec == SPEC_e)
      {
        // scientific notation
        // not implemented
      }
      else if (t.spec == SPEC_G || t.spec == SPEC_g)
      {
        // shortest of e/E or f
        // not implemented
      }
      else if (t.spec == SPEC_n)
      {
        // the number of characters printed so far
        // not implemented
      }
      else
      {
        // invalid specifier
        err = 1;
      }

    }
    else
    {
      fputc(*format, stream);
    }

    format++;
  }

  return 1;
}

#endif