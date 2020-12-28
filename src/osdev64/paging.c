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

  // TODO: verify support for 1 GiB pages using MTRRs.

  // uint64_t max_e = 0;
  // uint64_t max_phys = 0;
  // uint64_t gbpage = 0;

  // max_e = k_cpuid_rax(0x80000000);
  // printf("Max E: %X\n", max_e);

  // if (max_e >= 0x80000008)
  // {
  //   // Attempt to find the MAXPHYADDR bit count by using
  //   // CPUID.80000008H:EAX[7:0]
  //   max_phys = k_cpuid_rax(0x80000008) & 0xFF;
  //   printf("MAXPHYADDR: %llu\n", max_phys);

  //   // Check for 1 GiB page support
  //   // CPUID.80000001H:EDX.Page1GB [bit 26] 
  //   gbpage = k_cpuid_rdx(0x80000001);
  //   printf("1 GiB Page Support: %c\n", (gbpage & 0x4000000) ? 'Y' : 'N');
  // }
  // else
  // {
  //   printf("CPUID.80000008H:EAX[7:0] not supported\n");
  // }



  // uint64_t has_mtrr = k_cpuid_rdx(0x01);
  // printf("MSR: %c\n", (has_mtrr & 0x20) ? 'Y' : 'N');
  // printf("MTRR: %c\n", (has_mtrr & 0x1000) ? 'Y' : 'N');
  // printf("PAT: %c\n", (has_mtrr & 0x10000) ? 'Y' : 'N');

  // uint64_t pat = k_read_pat();
  // fprintf(stddbg, "PA0: %s\n", pat_to_str((pat & 0x7)));
  // fprintf(stddbg, "PA1: %s\n", pat_to_str(((pat >> 8) & 0x7)));
  // fprintf(stddbg, "PA2: %s\n", pat_to_str(((pat >> 16) & 0x7)));
  // fprintf(stddbg, "PA3: %s\n", pat_to_str(((pat >> 24) & 0x7)));
  // fprintf(stddbg, "PA4: %s\n", pat_to_str(((pat >> 32) & 0x7)));
  // fprintf(stddbg, "PA5: %s\n", pat_to_str(((pat >> 40) & 0x7)));
  // fprintf(stddbg, "PA6: %s\n", pat_to_str(((pat >> 48) & 0x7)));
  // fprintf(stddbg, "PA7: %s\n", pat_to_str(((pat >> 56) & 0x7)));

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
