#include "osdev64/core.h"
#include "osdev64/bitmask.h"
#include "osdev64/descriptor.h"
#include "osdev64/memory.h"
#include "osdev64/pic.h"

#include "klibc/stdio.h"

// The IDT contains interrupt gate descriptors.
// Each descriptor locates an ISR and specifies the offset of the segment
// descriptor in the GDT.

void isr0();
void isr1();
void isr2();
void isr3();
void isr4();
void isr5();
void isr6();
void isr7();
void isr8();
void isr9();
void isr10();
void isr11();
void isr12();
void isr13();
void isr14();
void isr15();
void isr16();
void isr17();
void isr18();
void isr19();
void isr20();
void isr21();
void isr22();
void isr23();
void isr24();
void isr25();
void isr26();
void isr27();
void isr28();
void isr29();
void isr30();
void isr31();

void generic_isr();

void generic_handler()
{
  fprintf(stddbg, "[INT] generic ISR\n");
}

// Currently used to handle all IRQs from the PIC.
void pic_handler()
{
  // fprintf(stddbg, "[PIC] interrupt handler\n");
}

// the IDT
int_desc* g_idt;


// Inserts an ISR into the IDT
void k_install_isr(uint64_t r, int i)
{
  int_desc lo = 0; // low 64 bits of descriptor
  int_desc hi = 0; // high 64 bits of descriptor

  // Bits [31:16] are the offset, in bytes, of the code segment selector.
  lo |= ((uint64_t)8 << 16);

  // Bits [34:32] are the IST index, ranging from 1-7 if an IST
  // is used, or 0 if no IST is used.
  // ISR0 will use IST1 for handling divide errors.
  // All other ISRs will just not bother with the IST for now.
  if (i == 0)
  {
    lo |= ((uint64_t)1 << 32);
  }

  // Bits [43:40] are the type configuration for a 32-bit interrupt gate.
  // We're currently using the bits 1 1 1 0
  lo |= (SG_SEG_INT_GATE << 40);

  // Bits [46:45] are the descriptor privilege level.
  // Currently, the segments described by the GDT all use DPL 0.
  lo |= ((uint64_t)0 << 45);

  // Set the present flag.
  lo |= BM_47;

  // The ISR address is placed in bits [15:0], [63:48], and [95:64]
  lo |= (r & 0xFFFF);                     // [15:0]
  lo |= ((r & 0xFFFF0000) << 32);         // [63:48]
  hi |= ((r & 0xFFFFFFFF00000000) >> 32); // [95:64]

  // Put the descriptor in the IDT.
  g_idt[i * 2] = lo;
  g_idt[i * 2 + 1] = hi;
}

// an assmebly procedure that executes the LIDT instruction
void k_lidt(uint16_t, int_desc*);

void k_idt_init()
{
  // Reserve memory for the IDT.
  // 4 KiB should be sufficient, since we allow up to 256
  // 128-bit entries, which works out to be exactly 4096 bytes.
  g_idt = (int_desc*)k_memory_alloc_pages(1);
  if (g_idt == NULL)
  {
    fprintf(stddbg, "[ERROR] failed to allocated memory for IDT\n");
    for (;;);
  }

  k_install_isr((uint64_t)isr0, 0);
  k_install_isr((uint64_t)isr1, 1);
  k_install_isr((uint64_t)isr2, 2);
  k_install_isr((uint64_t)isr3, 3);
  k_install_isr((uint64_t)isr4, 4);
  k_install_isr((uint64_t)isr5, 5);
  k_install_isr((uint64_t)isr6, 6);
  k_install_isr((uint64_t)isr7, 7);
  k_install_isr((uint64_t)isr8, 8);
  k_install_isr((uint64_t)isr9, 9);
  k_install_isr((uint64_t)isr10, 10);
  k_install_isr((uint64_t)isr11, 11);
  k_install_isr((uint64_t)isr12, 12);
  k_install_isr((uint64_t)isr13, 13);
  k_install_isr((uint64_t)isr14, 14);
  k_install_isr((uint64_t)isr15, 15);
  k_install_isr((uint64_t)isr16, 16);
  k_install_isr((uint64_t)isr17, 17);
  k_install_isr((uint64_t)isr18, 18);
  k_install_isr((uint64_t)isr19, 19);
  k_install_isr((uint64_t)isr20, 20);
  k_install_isr((uint64_t)isr21, 21);
  k_install_isr((uint64_t)isr22, 22);
  k_install_isr((uint64_t)isr23, 23);
  k_install_isr((uint64_t)isr24, 24);
  k_install_isr((uint64_t)isr25, 25);
  k_install_isr((uint64_t)isr26, 26);
  k_install_isr((uint64_t)isr27, 27);
  k_install_isr((uint64_t)isr28, 28);
  k_install_isr((uint64_t)isr29, 29);
  k_install_isr((uint64_t)isr30, 30);
  k_install_isr((uint64_t)isr31, 31);

  for (int i = 32; i < 256; i++)
  {
    k_install_isr((uint64_t)generic_isr, i);
  }

  uint16_t limit = (sizeof(int_desc) * 512) - 1;

  k_lidt(limit, g_idt);
}
