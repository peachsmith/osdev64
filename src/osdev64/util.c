#include "osdev64/util.h"


uint64_t k_align_4k(uint64_t addr)
{
  while (addr % 0x1000)
  {
    addr--;
  }

  return addr;
}
