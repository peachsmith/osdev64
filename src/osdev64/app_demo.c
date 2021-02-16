#include "osdev64/app_demo.h"
#include "osdev64/memory.h"

#include "klibc/stdio.h"

// ELF file loaded by the firmware.
// It is assumed that this file fits within 8192 bytes.
extern k_byte* g_app_bin;

void k_app_demo()
{
  // Print the ELF header.
  // It should be the number 0x75 followed by the string "ELF"
  fprintf(
    stddbg,
    "ELF Header: %X, %c%c%c\n",
    g_app_bin[0],
    g_app_bin[1],
    g_app_bin[2],
    g_app_bin[3]
  );

  
}
