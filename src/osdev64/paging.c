#include "osdev64/paging.h"
#include "osdev64/instructor.h"
#include "osdev64/cpuid.h"
#include "osdev64/control.h"
#include "osdev64/msr.h"

#include "klibc/stdio.h"

#include <stdint.h>


// PML4
pml4e pml4[512] __attribute__((aligned(0x1000)));

// PDPT
pdpte pdpt[512] __attribute__((aligned(0x1000)));

static char* pat_Uncacheable = "UC";
static char* pat_WriteCombining = "WC";
static char* pat_WriteThrough = "WT";
static char* pat_WriteProtected = "WP";
static char* pat_WriteBack = "WB";
static char* pat_Uncached = "UC-";
static char* pat_RES = "RES";

static inline char* pat_to_str(uint64_t p)
{
  switch (p)
  {
  case 0:
    return pat_Uncacheable;

  case 1:
    return pat_WriteCombining;

  case 4:
    return pat_WriteThrough;

  case 5:
    return pat_WriteProtected;

  case 6:
    return pat_WriteBack;

  case 7:
    return pat_Uncached;

  default:
    return pat_RES;
  }
}

// page sizes in hexadecimal
// 1 GiB = 0x40000000
// 2 MiB = 0x200000
// 4 KiB = 0x1000


// 1 GiB is 512 2 MiB pages

/**
 * Creates a PML4 entry to hold the address of a PDPT.
 * The resulting PML4 entry is marked as present, read/write,
 * and supervisor mode.
 * Page-level write through and page-level cache disable bits
 * are left at 0.
 *
 *
 * Params:
 *   uint64_t - the address of a PDPT
 *
 * Returns:
 *   pml4e - a PML4 entry
 */
pml4e make_pml4e(uint64_t pdpt_addr)
{
  pml4e p = 0;
  uint64_t bit_mask = 0x1; // single bit mask

  // Bit 0 is the present flag.
  // Mark the page as present.
  p |= (bit_mask << 0);

  // Bit 1 is the read/write flag.
  // Set this entry to be read/write.
  p |= (bit_mask << 1);

  // Bit 2 is the user/supvisor mode flag.
  // Leaving it at 0 since there's no user mode yet.

  // Bit 3 is the page-level write through bit.

  // Bit 4 is the page-level cache disabled bit.

  // Bit 5 is the access bit.

  // Bit 6 is ignored.

  // Bit 7 is reserved, and must be 0.

  // Bits [11:8] are ignored.

  // Bits [51:12] are the address of the PDPT entry.
  // Since this is a multiple of 4096, the lowest 12 bits
  // of the address should always be 0, so they shouldn't
  // change the existing bits of the PML4 entry.
  p |= pdpt_addr;

  // Bits [62:52] are ignored

  // Bit 63 is the execute-disable bit.
  // It's only used if IA32_EFER.NXE = 1, but we'll
  // leave it as 0 here.

  return p;
}


/**
 * Creates a PDPT entry.
 * The resulting PDPT entry is marked as present, read/write,
 * and supervisor mode.
 * Page-level write through and page-level cache disable bits
 * are left at 0.
 * The PAT and global translation bits are left at 0.
 * The PKE bits [62:59] are left as 0 since CR4.PKE is 0.
 *
 * Params:
 *   uint64_t - base address of 1 Gib page
 *
 * Returns:
 *   pdpte - a PDPT entry
 */
pdpte make_pdpte(uint64_t page_base)
{
  pdpte p = 0;
  uint64_t bit_mask = 0x1; // single bit mask

  // Bit 0 is the present flag.
  // Mark the page as present.
  p |= (bit_mask << 0);

  // Bit 1 is the read/write flag.
  // Set this entry to be read/write.
  p |= (bit_mask << 1);

  // Bit 2 is the user/supvisor mode flag.
  // Leaving it at 0 since there's no user mode yet.

  // Bit 3 is the page-level write through bit.

  // Bit 4 is the page-level cache disabled bit.

  // Bit 5 is the access flag.

  // Bit 6 is the dirty flag.

  // Bit 7 is the page size bit and must be set
  // in order for the PDPT entry to point to a 1 GiB page.
  p |= (bit_mask << 7);

  // Bit 8 is the global flag.

  // Bits [11:9] are ignored.

  // Bit 12 is the PAT bit.

  // Bits [51:30] are the base address of a 1 GiB page frame.
  p |= (page_base);

  // Bits [58:52] are ignored

  // Bits [62:59] are the protection key.
  // This is only applicable if CR4.PKE = 1.

  // bit 63 is the execute-disable bit.
  // It's only used if IA32_EFER.NXE = 1, but we'll
  // leave it as 0 here.

  return p;
}


void k_paging_init()
{
  // The UEFI firmware should have identity mapped  the first 4 GiB
  // of address space. We will attempt to do that here as well.

  // One way to check for 1 GiB page support is to use
  // CPUID.80000001H:EDX.Page1GB [bit 26]


  // Read the IA32_MTRRCAP MSR
  uint64_t mtrrcap = k_get_mtrrcap();
  uint64_t vcnt = mtrrcap & 0xFF;
  uint64_t fixed = mtrrcap & BM_10;
  uint64_t smrr = mtrrcap & BM_11;

  fprintf(stddbg, "[MTRR] VCNT:  %-llu\n", vcnt);
  fprintf(stddbg, "[MTRR] Fixed: %c\n", (mtrrcap & BM_8) ? 'Y' : 'N');
  fprintf(stddbg, "[MTRR] WC:    %c\n", fixed ? 'Y' : 'N');
  fprintf(stddbg, "[MTRR] SMRR:  %c\n", smrr ? 'Y' : 'N');

  


  // Identity map 512 GiB of address space in the PDPT.
  uint64_t phys = 0;
  for (uint64_t i = 0; i < 512; i++)
  {
    pdpt[i] = make_pdpte(i * 0x40000000);
  }

  // Put the PDPT in the PML4.
  pml4[0] = make_pml4e((uint64_t)(pdpt));

  // Mark the rest of the entries in the PML4 as not present.
  for (int i = 1; i < 512; i++)
  {
    pml4[i] = pml4[0] & ~((uint64_t)1);
  }

  uint64_t cr3 = k_get_cr3();

  // Put the address of our PML4 in CR3.
  cr3 = (uint64_t)pml4;

  // Update CR3.
  k_set_cr3(cr3);
}
