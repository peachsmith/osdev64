#include "osdev64/uefi.h"
#include "osdev64/descriptor.h"


// the number of entries in the GDT
#define GDT_COUNT 5

// GDT
seg_desc gdt[GDT_COUNT];

// TSS
uint32_t tss[26];

// IST1 stack
// TODO: get this from RAM
uint8_t ist1[8192]__attribute__((aligned(16)));

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

  // Set the base address.
  desc |= ((base & 0xFF000000) << 32);
  desc |= ((base & 0x00FFFFFF) << 16);

  // Set the limit.
  desc |= ((limit & 0x0F0000) << 32);
  desc |= (limit & 0x00FFFF);

  // Set the granularity to be 4KB.
  desc |= ((uint64_t)0x1 << 55);

  // Make sure the default operand size flag is 0
  // since we're in 64-bit mode.
  desc |= ((uint64_t)0x0 << 54);

  // Set the long mode flag.
  desc |= ((uint64_t)0x1 << 53);

  // Set the system use bit to 0.
  // We're not currently using this.
  desc |= ((uint64_t)0x0 << 52);

  // Set the presence flag.
  desc |= ((uint64_t)0x1 << 47);

  // Set the descriptor privilege level to 0.
  // Give us all the permissions!
  desc |= ((uint64_t)0x0 << 45);

  // Set the type flag to 1 since this descriptor
  // is describing a code or data segment.
  desc |= ((uint64_t)0x1 << 44);

  // Set the type field.
  desc |= ((type & 0x0F) << 40);

  return desc;
}

// an assmebly procedure that executes the LGDT instruction
void k_lgdt(uint16_t, seg_desc*);

// an assembly procedure that executes the LTR instruction
void k_ltr(uint16_t);

void k_load_gdt()
{
  gdt[0] = 0; // null descriptor

  // code segment descriptor
  // type: execute/read, conforming, not yet accessed
  gdt[1] = build_cd_descriptor(0, 0x0FFFFF, CD_SEG_ERC);

  // data segment descriptor
  // type: read/write, expand down, not yet accessed
  gdt[2] = build_cd_descriptor(0, 0x0FFFFF, CD_SEG_RWD);

  // Initialize all the values of the TSS to 0.
  for (int i = 0; i < 26; i++)
  {
    tss[i] = 0;
  }

  // Put the address of IST1 in the TSS?
  tss[9] = (uint32_t)(((uint64_t)(ist1 + (sizeof(uint8_t) * 4096)) & 0xFFFFFFFF));
  tss[10] = (uint32_t)(((uint64_t)(ist1 + (sizeof(uint8_t) * 4096)) & 0xFFFFFFFF00000000) >> 32);

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
  tss_lo |= (((uint64_t)tss & 0xFF000000) << 32);
  tss_lo |= (((uint64_t)tss & 0x00FFFFFF) << 16);
  tss_hi |= (((uint64_t)tss & 0xFFFFFFFF00000000) >> 32);

  // Put the TSS descriptor in the GDT.
  gdt[3] = tss_lo;
  gdt[4] = tss_hi;

  uint16_t limit = sizeof(gdt) - 1;

  // load the GDT
  k_lgdt(limit, gdt);

  // load the TSS
  k_ltr(0x18);
}
