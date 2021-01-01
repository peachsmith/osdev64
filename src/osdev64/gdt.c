#include "osdev64/core.h"
#include "osdev64/uefi.h"
#include "osdev64/bitmask.h"
#include "osdev64/descriptor.h"
#include "osdev64/memory.h"

#include "klibc/stdio.h"

// GDT
// Currently expects 5 entries
seg_desc* g_gdt;

// TSS
// uint32_t g_tss[TSS_COUNT];
uint32_t* g_tss;

// IST1 stack
unsigned char* g_ist1;

/**
 * Creates a segment descriptor which describes a 64-bit code or data
 * segment.
 *
 * Params:
 *   base - 32 bits containing the logical base of the segment
 *   limit - 20 bits containing the size of the segment in units of 4KB
 *   type - 4 bits containing the descriptor type field.
 *
 * Returns:
 *   seg_desc - a code or data segment descriptor
 */
static seg_desc build_cd_descriptor
(
  uint64_t base,
  uint64_t limit,
  cd_seg_type type
)
{
  // Create a segment descriptor.
  seg_desc desc = 0;

  // The base address of the segment is put into bits [31:16], [39:32],
  // and [63:56]. Since we're in 64-bit mode, we can treat bits [39:16]
  // as a single, contiguous collection of bits.
  desc |= ((base & 0x00FFFFFF) << 16); // [39:16]
  desc |= ((base & 0xFF000000) << 32); // [63:56]

  // The limit of the segment is put into bits [15:0] and [51:48].
  desc |= (limit & 0x00FFFF);         // [15:0]
  desc |= ((limit & 0x0F0000) << 32); // [51:48]

  // Set the granularity to be 4KB.
  desc |= BM_55;

  // Bit 54 is the default operand size flag.
  // It should be 0 since we're in 64-bit mode.

  // Bit 53 is the long mode flag.
  // This should be 1 since we're in 64-bit mode.
  desc |= BM_53;

  // Bit 52 is the system use bit.
  // We're leaving it as 0.

  // Bit 47 is the present flag.
  desc |= BM_47;

  // Bits [46:45] are the descriptor privilege level.
  // We'll set these to 0 for now for DPL 0.
  desc |= ((uint64_t)0 << 45);

  // Bit 44 is the type flag.
  // We set this to 1 to indicate that this descriptor describes
  // a code or data segment.
  desc |= BM_44;

  // Bits [43:40] are the descriptor type configuration.
  desc |= ((type & 0x0F) << 40);

  return desc;
}

// an assmebly procedure that executes the LGDT instruction
void k_lgdt(uint16_t, seg_desc*);

// an assembly procedure that executes the LTR instruction
void k_ltr(uint16_t);

void k_load_gdt()
{
  // Reserve some memory for the GDT.
  // 4 KiB should be more than enough.
  g_gdt = (seg_desc*)k_memory_alloc_pages(1);
  if (g_gdt == NULL)
  {
    fprintf(stddbg, "[ERROR] failed to allocate memory for GDT\n");
    for (;;);
  }

  uint16_t gdt_count = 0;

  g_gdt[gdt_count++] = 0; // null descriptor

  // code segment descriptor
  // type: execute/read, conforming, not yet accessed
  g_gdt[gdt_count++] = build_cd_descriptor(0, 0x0FFFFF, CD_SEG_ERC);

  // data segment descriptor
  // type: read/write, expand down, not yet accessed
  g_gdt[gdt_count++] = build_cd_descriptor(0, 0x0FFFFF, CD_SEG_RWD);


  // Reserve memory for the TSS.
  // The TSS contains 26 32-bit entries, so 4 KiB should be plenty.
  g_tss = (uint32_t*)k_memory_alloc_pages(1);
  if (g_tss == NULL)
  {
    fprintf(stddbg, "[ERROR] failed to allocate memory for TSS\n");
    for (;;);
  }

  // Initialize all the values of the TSS to 0.
  for (int i = 0; i < TSS_COUNT; i++)
  {
    g_tss[i] = 0;
  }

  // Reserve two pages for IST1.
  // Since a stack grows down (top of stack at lower address),
  // the beginning of the IST stack is the base of the second page.
  // So if we allocated two pages starting at address 0xA000, then
  // the beginning of the stack would be 0xB000.
  g_ist1 = (unsigned char*)k_memory_alloc_pages(2);
  if (g_ist1 == NULL)
  {
    fprintf(stddbg, "[ERROR] failed to allocate memory for IST1\n");
    for (;;);
  }

  // Put IST1 in the TSS.
  g_tss[9] = (uint32_t)(((uint64_t)(g_ist1 + 4096) & 0xFFFFFFFF));
  g_tss[10] = (uint32_t)(((uint64_t)(g_ist1 + 4096) & 0xFFFFFFFF00000000) >> 32);

  // In 64-bit mode, a TSS descriptor is 128 bits, so we use two descriptors
  // to represent the low and high bits.
  seg_desc tss_lo = 0;
  seg_desc tss_hi = 0;

  // Configure the TSS descriptor
  // 32-bit available/busy 0x9/0xB
  // byte/4KB 0x0/0x1
  tss_lo |= ((uint64_t)0xFFFF); // limit (low 16 bits)
  tss_lo |= ((uint64_t)0x0F0000 << 32); // limit (high 4 bits)
  tss_lo |= (SG_SEG_TSS_AVL << 40); // type:        1001 available
  tss_lo |= ((uint64_t)0x1 << 47);  // present:     true
  tss_lo |= ((uint64_t)0x1 << 55);  // granularity: 4KB

  // The TSS base address is split over the high and low
  // descriptor portions.
  tss_lo |= (((uint64_t)g_tss & 0xFF000000) << 32);
  tss_lo |= (((uint64_t)g_tss & 0x00FFFFFF) << 16);
  tss_hi |= (((uint64_t)g_tss & 0xFFFFFFFF00000000) >> 32);

  // Put the TSS descriptor in the GDT.
  g_gdt[gdt_count++] = tss_lo;
  g_gdt[gdt_count++] = tss_hi;

  uint16_t limit = (sizeof(seg_desc) * gdt_count) - 1;

  // load the GDT
  k_lgdt(limit, g_gdt);

  // load the TSS
  k_ltr(0x18);
}
