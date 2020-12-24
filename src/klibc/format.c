#ifndef JEP_FORMAT_C
#define JEP_FORMAT_C

/**
 * This file contains utility functions for handling format strings
 * used by the printf family of functions.
 */

;

#include "klibc/stdio.h"
#include "klibc/string.h"

#include <stdint.h>
#include <stddef.h>

// format flag bit flags
#define FMT_LEFT   0x01 /* -                               */
#define FMT_SIGN   0x02 /* +                               */
#define FMT_SPACE  0x04 /* [space]                         */
#define FMT_POINT  0x08 /* #                               */
#define FMT_ZERO   0x10 /* 0                               */
#define FMT_WIDTH  0x20 /* width is passed as argument     */
#define FMT_PREC   0x40 /* precision is passed as argument */
#define FMT_ZPREC  0x40 /* precision is zero               */

// format specifiers
#define SPEC_c 'c'
#define SPEC_s 's'
#define SPEC_d 'd'
#define SPEC_i 'i'
#define SPEC_u 'u'
#define SPEC_f 'f'
#define SPEC_e 'e'
#define SPEC_E 'E'
#define SPEC_g 'g'
#define SPEC_G 'G'
#define SPEC_o 'o'
#define SPEC_x 'x'
#define SPEC_X 'X'
#define SPEC_p 'p'
#define SPEC_n 'n'
#define SPEC_per '%'
#define SPEC_b 'b' // (binary) NOT standard

// format length bit flags
#define LEN_h  0x01 /* h  */
#define LEN_l  0x02 /* l  */
#define LEN_ll 0x04 /* ll */
#define LEN_L  0x08 /* L  */

// format tag parser states
#define STATE_FLAGS  1
#define STATE_WIDTH  2
#define STATE_PREC   3
#define STATE_LENGTH 4
#define STATE_SPEC   5
#define STATE_DONE   6

/**
 * Determines if a character within a format tag is a valid format flag.
 * If the character is a format flag, then a flag variable is set to have
 * the value of one of the format bit flags.
 * If the character is not a valid flag value, the variable is set to 0.
 *
 * Params:
 *   c - a character within a format tag
 *   f - a variable to contain the bit flag
 */
#define is_flag(c, f) (     \
    f = c == '-' ? FMT_LEFT \
    : c == '+' ? FMT_SIGN   \
    : c == ' ' ? FMT_SPACE  \
    : c == '#' ? FMT_POINT  \
    : c == '0' ? FMT_ZERO   \
    : 0                     \
)

;

/**
 * Determines if a character within a format tag is a valid length value.
 * If the character is a valid length value, then a flag variable is set to
 * have the value of one of the length bit flags.
 * If the character is not a valid length value, the variable is set to 0.
 *
 * Params:
 *   c - a character within a format tag
 *   l - a variable to contain the bit flag
 */
#define is_len(c, l) (   \
    l = c == 'h' ? LEN_h \
    : c == 'l' ? LEN_l   \
    : c == 'L' ? LEN_L   \
    : 0                  \
)

;

/**
 * Determines if a character within a format tag is a valid specifier.
 * If the character is a valid specifier, then a variable is set with the
 * value of that specifier.
 * If the character is not a valid specifier, the variable is set to 0.
 *
 * Params:
 *   c - a character within a format tag
 *   s - a variable to contain the specifier character
 */
#define is_spec(c, s) (   \
    s = c == 'c' ? SPEC_c \
    : c == 's' ? SPEC_s   \
    : c == 'd' ? SPEC_d   \
    : c == 'i' ? SPEC_i   \
    : c == 'u' ? SPEC_u   \
    : c == 'f' ? SPEC_f   \
    : c == 'e' ? SPEC_e   \
    : c == 'E' ? SPEC_E   \
    : c == 'g' ? SPEC_g   \
    : c == 'G' ? SPEC_G   \
    : c == 'o' ? SPEC_o   \
    : c == 'x' ? SPEC_x   \
    : c == 'X' ? SPEC_X   \
    : c == 'p' ? SPEC_p   \
    : c == 'n' ? SPEC_n   \
    : c == '%' ? SPEC_per \
    : c == 'b' ? SPEC_b   \
    : 0                   \
)

;

/**
 * A format tag within a format string.
 */
typedef struct ftag {
  unsigned char flags;
  size_t width;
  size_t prec;
  unsigned char len;
  char spec;
}ftag;


/**
 * Parses a string of decimal digits as an unsigned decimal integer.
 *
 * Params:
 *   const char* - a string of digit characters
 *   size_t* - a pointer to the parsed number
 *
 * Returns:
 *   size_t - the number of characters parsed
 */
static size_t parse_udec(const char* str, size_t* res)
{
  size_t count;
  size_t n;
  size_t tmp;
  size_t err;

  count = n = err = 0;

  while (*str >= '0' && *str <= '9' && !err)
  {
    tmp = (size_t)(*str) - '0';

    if (n == 0 && count == 1)
    {
      err = 1;
    }
    else
    {
      n = n * 10 + tmp;
      str++;
      count++;
    }
  }

  *res = n;

  return count;
}

/**
 * Parses a format tag from a source string used by the printf family
 * of functions.
 * Upone completion, a pointer to the current position in the source
 * string is updated with the location of the end of the format tag.
 *
 * Params:
 *   const char* - the starting point in the source string
 *   char** - a pointer to the end of the format string
 */
static ftag parse_format(const char* start, char** end)
{
  size_t n;           // conversion result
  size_t d;           // index change
  int state;          // current parser state
  unsigned char flag; // flag
  unsigned char len;  // length
  char spec;          // specifier
  ftag tag;           // format tag
  size_t e;           // element size

  tag.flags = 0;
  tag.width = 0;
  tag.prec = 0;
  tag.len = 0;
  tag.spec = 0;

  state = STATE_FLAGS;
  e = sizeof(char);

  while (*start != '\0' && state != STATE_DONE)
  {
    switch (state)
    {
    case STATE_FLAGS:
      if (is_flag(*start, flag))
      {
        tag.flags |= flag;
        start++;
      }
      else
      {
        state++;
      }
      break;

    case STATE_WIDTH:
      if (*start == '*')
      {
        tag.flags |= FMT_WIDTH;
        start++;
      }
      else
      {
        d = parse_udec(start, &n);
        tag.width = n;
        start += e * d;
      }
      state++;
      break;

    case STATE_PREC:
      if (*start == '.')
      {
        start += e;
        if (*start == '*')
        {
          tag.flags |= FMT_PREC;
          start += e;
        }
        else
        {
          d = parse_udec(start, &n);
          tag.prec = n;
          start += e * d;
        }
      }
      state++;
      break;

    case STATE_LENGTH:
      if (is_len(*start, len))
      {
        if (len == LEN_l && tag.len & LEN_l)
        {
          tag.len |= LEN_ll;
        }
        else
        {
          tag.len |= len;
        }
        start += e;
      }
      else
      {
        state++;
      }

      break;

    case STATE_SPEC:
      tag.spec = is_spec(*start, spec) ? spec : 0;
      state++;
      break;

    default:
      break;
    }
  }

  // Update the end position.
  *end = (char*)start;

  return tag;
}

/**
 * Reverses the order of a character array.
 * Given a length of l, only the first l characters will be reversed.
 * Any characters after that in the buffer will remain unmoved.
 *
 * Params:
 *   char* - a pointer to a character array
 *   size_t - the number of characters to reverse
 */
static void reverse(char* str, size_t len)
{
  size_t beg; // beginning of string
  size_t end; // end of string
  char tmp;   // temporary character storage

  for (beg = 0, end = len - 1; beg < end;)
  {
    tmp = str[beg];
    str[beg++] = str[end];
    str[end--] = tmp;
  }
}

static size_t int_to_buffer(int n, char* buffer, ftag t)
{
  size_t i;       // index
  int radix;      // base (8, 10, or 16)
  int sign;       // whether or not the integer is signed
  int cap;        // whether to use capitalized hex digits
  int r;          // remainder
  int orig;       // original integer value
  unsigned int u; // unsigned integer

  // Ensure that the buffer is not a NULL pointer.
  if (buffer == NULL)
  {
    return 0;
  }

  // Only allow integer formats.
  if (t.spec == SPEC_o)
  {
    radix = 8;
  }
  else if (t.spec == SPEC_i || t.spec == SPEC_d || t.spec == SPEC_u)
  {
    radix = 10;
  }
  else if (t.spec == SPEC_X || t.spec == SPEC_x || t.spec == SPEC_p)
  {
    radix = 16;
  }
  else
  {
    return 0;
  }

  // Save the original value of n.
  orig = n;

  // Check for capitalization.
  cap = t.spec == SPEC_X;

  // Check whether we're dealing with a signed integer.
  sign = t.spec == SPEC_d || t.spec == SPEC_i;

  // Convert the integer to an unsigned integer
  // for the purpose of evaluating the individual digits.
  u = sign ? (n < 0 ? -n : n) : (unsigned int)n;

  i = 0;

  // If the integer is 0, we only need one digit,
  // so we populate the array accordingly and return.
  if (n == 0)
  {
    buffer[i++] = '0';
  }

  while (u)
  {
    r = u % radix;
    buffer[i++] = r > 9 ? r - 10 + (cap ? 'A' : 'a') : r + '0';
    u /= radix;
  }

  // If the format tag contains the '#' flag,
  // then we add the lead '0' or "0x" depending on the radix.
  if (t.flags & FMT_POINT)
  {
    if (radix == 16)
    {
      buffer[i++] = cap ? 'X' : 'x';
      buffer[i++] = '0';
    }
    else if (radix == 8)
      buffer[i++] = '0';
  }

  // Append the negative sign if necessary.
  if (orig < 0 && sign)
    buffer[i++] = '-';

  // Terminate the string with the NUL character.
  buffer[i] = '\0';

  // The resulting string is backwards, so we need to reverse it.
  reverse(buffer, i);

  return i;
}

static size_t int64_to_buffer(int64_t n, char* buffer, ftag t)
{
  size_t i;   // index
  int radix;  // base (8, 10, or 16)
  int sign;   // whether or not the integer is signed
  int cap;    // whether to use capitalized hex digits
  int r;      // remainder
  int orig;   // original integer value
  uint64_t u; // unsigned integer

  // Ensure that the buffer is not a NULL pointer.
  if (buffer == NULL)
  {
    return 0;
  }

  // Only allow integer formats.
  if (t.spec == SPEC_o)
  {
    radix = 8;
  }
  else if (t.spec == SPEC_i || t.spec == SPEC_d || t.spec == SPEC_u)
  {
    radix = 10;
  }
  else if (t.spec == SPEC_X || t.spec == SPEC_x || t.spec == SPEC_p)
  {
    radix = 16;
  }
  else
  {
    return 0;
  }

  // Save the original value of n.
  orig = n;

  // Check for capitalization.
  cap = t.spec == SPEC_X;

  // Check whether we're dealing with a signed integer.
  sign = t.spec == SPEC_d || t.spec == SPEC_i;

  // Convert the integer to an unsigned integer
  // for the purpose of evaluating the individual digits.
  u = sign ? (n < 0 ? -n : n) : (uint64_t)n;

  i = 0;

  // If the integer is 0, we only need one digit,
  // so we populate the array accordingly and return.
  if (n == 0)
  {
    buffer[i++] = '0';
    buffer[i] = '\0';
    return 1;
  }

  while (u)
  {
    r = u % radix;
    buffer[i++] = r > 9 ? r - 10 + (cap ? 'A' : 'a') : r + '0';
    u /= radix;
  }

  // If the format tag contains the '#' flag,
  // then we add the lead '0' or "0x" depending on the radix.
  if (t.flags & FMT_POINT)
  {
    if (radix == 16)
    {
      buffer[i++] = cap ? 'X' : 'x';
      buffer[i++] = '0';
    }
    else if (radix == 8)
    {
      buffer[i++] = '0';
    }
  }

  // Append the negative sign if necessary.
  if (orig < 0 && sign)
  {
    buffer[i++] = '-';
  }

  // Terminate the string with the NUL character.
  buffer[i] = '\0';

  // The resulting string is backwards, so we need to reverse it.
  reverse(buffer, i);

  return i;
}


static size_t uptr_to_buffer(uintptr_t n, char* buffer, int cap)
{
  size_t i; // index
  int r;    // remainder

  // Ensure that the buffer is not a NULL pointer
  if (buffer == NULL)
  {
    return 0;
  }

  i = 0;

  // If the integer is 0, we only need one digit,
  // so we populate the array accordingly and return.
  if (n == 0)
  {
    buffer[i++] = '0';
    buffer[i] = '\0';
    return 1;
  }

  while (n)
  {
    r = n % 16;
    buffer[i++] = r > 9 ? r - 10 + (cap ? 'A' : 'a') : r + '0';
    n /= 16;
  }

  // Terminate the string with the NUL character.
  buffer[i] = '\0';

  // The resulting string is backwards, so we need to reverse it.
  reverse(buffer, i);

  return i;
}


static size_t bin_to_buffer(int64_t n, char* buffer, int prec)
{
  size_t i;   // index
  int sign;   // whether or not the integer is signed
  int r;      // remainder
  int orig;   // original integer value
  uint64_t u; // unsigned integer

  // Ensure that the buffer is not a NULL pointer.
  if (buffer == NULL)
  {
    return 0;
  }

  // Save the original value of n.
  orig = n;

  // Convert the integer to an unsigned integer
  // for the purpose of evaluating the individual digits.
  u = sign ? (n < 0 ? -n : n) : (uint64_t)n;

  i = 0;

  // If the integer is 0, we only need one digit,
  // so we populate the array accordingly and return.
  if (n == 0)
  {
    buffer[i++] = '0';
    buffer[i] = '\0';
    return 1;
  }

  while (u)
  {
    r = u % 2;
    buffer[i++] = r + '0';
    u /= 2;
  }

  // fill the rest of the buffer with padding
  while (i < prec)
  {
    buffer[i++] = '0';
  }

  // Terminate the string with the NUL character.
  buffer[i] = '\0';

  // The resulting string is backwards, so we need to reverse it.
  reverse(buffer, i);

  return i;
}


/**
 * Writes the contents of a character array to stdout using constraints
 * provided by a format tag.
 * The character array is expected to be a numerical value that has been
 * converted to a string.
 *
 * Params:
 *   char* - a pointer to a character array
 *   size_t - the length of the array
 *   ftag - the format tag
 */
static void print_num(FILE* stream, char* buf, size_t len, ftag t)
{
  int neg;     // whether or not the number is negative
  size_t i;    // index
  size_t d;    // width deifference
  size_t plen; // pointer length
  int point;   // whether the '#' flag is present
  int radix;   // the base (8, 10, or 16)

  neg = 0;
  plen = sizeof(uintptr_t) * 2;
  point = t.flags & FMT_POINT;

  if (t.spec == SPEC_o)
  {
    radix = 8;
  }
  else if (t.spec == SPEC_i || t.spec == SPEC_d || t.spec == SPEC_u)
  {
    radix = 10;
  }
  else if (t.spec == SPEC_X || t.spec == SPEC_x || t.spec == SPEC_p)
  {
    radix = 16;
  }
  else
  {
    radix = 10;
  }

  // Print the negative sign '-'
  if (len && buf[0] == '-')
  {
    neg = 1;
    fputc(buf[0], stream);
    t.prec++;
  }

  // Print the positive sign '+'
  if (len && buf[0] != '-' && (t.flags & FMT_SIGN))
  {
    fputc('+', stream);
    t.prec++;
    if (t.width)
    {
      t.width--;
    }
  }

  // Print a leading blank space if the space flag is set
  if (len && buf[0] != '-' && !(t.flags & FMT_SIGN) && (t.flags & FMT_SPACE))
  {
    fputc((t.flags & FMT_ZERO) ? '0' : ' ', stream);
    t.prec++;
    if (t.width)
    {
      t.width--;
    }
  }

  // Pad with leading spaces if the '-' flag is not present
  if (len < t.width && !(t.flags & FMT_LEFT))
  {
    if ((t.prec < t.width) && (t.spec != SPEC_p))
    {
      d = len > t.prec ? len : t.prec;
      for (i = 0; i < t.width - d; i++)
      {
        fputc(' ', stream);
      }
    }
    else if (t.spec == SPEC_p && plen < t.width)
    {
      for (i = 0; i < t.width - plen; i++)
      {
        fputc(' ', stream);
      }
    }
  }

  // Print the leading "0x" or "0X"
  if (t.flags & FMT_POINT)
  {
    if (t.spec == SPEC_X || t.spec == SPEC_x)
    {
      fputc(buf[0], stream);
      fputc(buf[1], stream);
      t.prec += 2;
    }
  }

  // Print leading zeros for pointers
  if (t.spec == SPEC_p && len < plen)
  {
    for (i = 0; i < plen - len; i++)
    {
      fputc('0', stream);
    }
  }

  // Pad with '0' for the precision
  if (len < t.prec && t.spec != SPEC_p)
  {
    for (i = len; i < t.prec; i++)
    {
      fputc('0', stream);
    }
  }

  // Determine the starting index for printing.
  if (neg)
  {
    i = 1;
  }
  else if (point && radix == 16 && t.spec != SPEC_p)
  {
    i = 2;
  }
  else
  {
    i = 0;
  }

  // Print the contents of the buffer.
  for (; i < len; i++)
  {
    fputc(buf[i], stream);
  }

  // Pad with trailing spaces if the '-' flag is present
  if (len < t.width && (t.flags & FMT_LEFT))
  {
    if ((t.prec < t.width) && (t.spec != SPEC_p))
    {
      d = len > t.prec ? len : t.prec;
      for (i = 0; i < t.width - d; i++)
      {
        fputc(' ', stream);
      }
    }
    else if (t.spec == SPEC_p && plen < t.width)
    {
      for (i = 0; i < t.width - plen; i++)
      {
        fputc(' ', stream);
      }
    }
  }
}

#endif