#ifndef JEP_STRING_C
#define JEP_STRING_C

#include "klibc/string.h"


void* memcpy(void* dest, const void* src, size_t n)
{
  size_t i;
  char* d = (char*)dest;
  const char* s = (char*)src;

  for (i = 0; i < n; i++)
  {
    d[i] = s[i];
  }

  return dest;
}


void* memmove(void* dest, const void* src, size_t n)
{
  size_t i;
  char* d = (char*)dest;
  const char* s = (char*)src;

  if (dest < src)
  {
    // copy forward
    for (i = 0; i < n; i++)
    {
      d[i] = s[i];
    }
  }
  else if (n > 0)
  {
    // copy backward
    for (i = n - 1; i > 0; i--)
    {
      d[i] = s[i];
    }

    d[i] = s[i];
  }

  return dest;
}


size_t strlen(const char* str)
{
  size_t len = 0;

  while (str[len])
  {
    len++;
  }

  return len;
}

#endif