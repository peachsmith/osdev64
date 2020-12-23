#ifndef JEP_PUTS_C
#define JEP_PUTS_C

#include "include/stdio.h"

int puts(const char* str)
{
  fputs(str, stdout);
}

int fputs(const char* str, FILE* stream)
{
  int res = 0;
  int cres = 0;
  while (*str != '\0')
  {
    if (!(cres = fputc(*str, stream)))
    {
      return -1;
    }
    str++;
    res++;
  }

  return res;
}

#endif