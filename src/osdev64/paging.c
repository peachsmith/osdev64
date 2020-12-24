#include "osdev64/paging.h"
#include "osdev64/instructor.h"

#include "klibc/stdio.h"

#include <stdint.h>



// TODO: if putting the paging structures in dynamic memory,
// be sure to page align them (base address should be multiple of 0x1000)
// Also, confirm what "alignment" means. I'm pretty sure it means
// that the base address of whatever you're aligning is a multiple
// of some number.

// PML4
uint64_t pml4[512] __attribute__((aligned(0x1000)));

// PDPT
uint64_t pdpt[512] __attribute__((aligned(0x1000)));


// page sizes in hexadecimal
// 1 GiB = 0x40000000
// 1 MiB = 0x100000
// 1 KiB = 0x1000

/**
 * Creates a PML4 entry to hold the address of a PDPT.
 * The resulting PML4 entry is marked as present, read/write,
 * and supervisor mode.
 *
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
  // Leaving it as 0 for now since I don't know what this is.
  // TODO: figure out what this means

  // Bit 4 is the page-level cache disabled bit.
  // Leaving it as 0 for now since I don't know what this is.
  // TODO: figure out what this means

  // Bit 5 is the access bit. We'll leave it as 0.
  // Bit 6 is ignored.
  // Bit 7 is reserved, and must be 0.
  // Bits [11:8] are ignored.

  // Set the address of the PDPT entry.
  // pdpt_addr &= mpa_mask;
  p |= (pdpt_addr << 12);

  // Bits [62:52] are ignored

  // Bit 63 is the execute-disable bit.
  // It's only used if IA32_EFER.NXE = 1, but we'll
  // leave it as 0 here.

  return p;
}

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
  // Leaving it as 0 for now since I don't know what this is.
  // TODO: figure out what this means

  // Bit 4 is the page-level cache disabled bit.
  // Leaving it as 0 for now since I don't know what this is.
  // TODO: figure out what this means

  // Bit 5 is the access flag.
  // Bit 6 is the dirty flag.

  // Bit 7 is the page size bit and must be set
  // in order for the PDPT entry to point to a 1 GiB page.
  p |= (bit_mask << 7);

  // Bit 8 is the global flag.
  // We'll clear it for now.
  // TODO: learn about global translation and whether it's
  // worth using.

  // Bits [11:9] are ignored.

  // Bit 12 is the PAT flag.
  // I have no idea what this is, so we're going to set this
  // to 0 for now.
  // TODO: figure this out

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

  // Attempt to find the MAXPHYADDR bit count by using
  // CPUID.80000008H:EAX[7:0]
  uint64_t max_e;
  uint64_t max_phys;

  max_e = k_cpuid_rax(0x80000000);
  printf("max E: %X\n", max_e);
  if (max_e >= 0x80000008)
  {
    max_phys = k_cpuid_rax(0x80000008) & 0xFF;
    printf("max phys: %llu\n", max_phys);
  }
  else
  {
    printf("CPUID.80000008H:EAX[7:0] not supported\n");
  }

  // Create a bit mask for MAXPHYADDR.
  // TODO: confirm likelyhood of MAXPHYADDR being 40.
  uint64_t mask = 0;
  for (uint64_t i = 0; i < max_phys; i++)
  {
    mask |= ((uint64_t)1 << i);
  }
  printf("MPA mask: %.64b\n", mask);



  // Identity map the first 4 GiB of address space.
  // TODO: confirm the likelyhood of the PDPT entry's
  // page address being 22 bits.
  uint64_t phys = 0;
  uint64_t phys_mask = 0xFFFFFC0000000; // mask to get the high 22 bits of an address
  for (uint64_t i = 0; i < 512; i++)
  {
    pdpt[i] = make_pdpte(i * 0x40000000);
  }

  // Put the PDPT in the PML4.
  pml4[0] = make_pml4e((uint64_t)(&pdpt));

  // Mark the rest of the entries in the PML4 as not present.
  for (int i = 1; i < 512; i++)
  {
    pml4e p = make_pml4e((uint64_t)(&pdpt));
    p &= ~((uint64_t)1);
  }

  // Get CR0 and CR3 since we'll need them to install our
  // paging structures.
  uint64_t cr0 = k_get_cr0();
  uint64_t cr3 = k_get_cr3();
  fprintf(stddbg, "UEFI's CR3: %p\n", cr3);

  // Disable paging
  // TODO: confirm if this is even necessary.
  // cr0 &= ~CR0_PG;
  // k_set_cr0(cr0);

  // Put the address of our PML4 in CR3.
  cr3 = (((uint64_t)(&pml4)) << 12);
  fprintf(stddbg, "new CR3: %p\n", cr3);

  // Bits [11:0] of CR3 should be 0.
  // [2:0] reserved
  // [4:3] page-level write through
  // [5]   page-level cache disable
  // [11:5] reserved
  // for (int i = 0; i <= 11; i++)
  // {
  //   cr3 &= ~((uint64_t)1 << i);
  // }

  // Update CR3.
  k_set_cr3(cr3);

  // Enable paging
  // TODO: confirm if this is even necessary.
  // cr0 |= CR0_PG;
  // k_set_cr0(cr0);

  // Confirm that CR3 has been updated.
  cr3 = k_get_cr3();
  fprintf(stddbg, "actual CR3: %p\n", cr3);

}
