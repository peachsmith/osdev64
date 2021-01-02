#include "osdev64/core.h"
#include "osdev64/bitmask.h"
#include "osdev64/paging.h"
#include "osdev64/instructor.h"
#include "osdev64/cpuid.h"
#include "osdev64/control.h"
#include "osdev64/msr.h"
#include "osdev64/memory.h"

#include "klibc/stdio.h"

#include <stdint.h>


extern uint64_t g_total_ram;


// Page Tables
pte* g_pt_mem;

// Page Directories
pde* g_pd_mem;

// PDPTs
pdpte* g_pdpt_mem;

// PML4
pml4e* g_pml4_mem;

typedef struct map_ledger_entry {
  uint64_t start;
  uint64_t end;
  uint16_t pdpt_start;
  uint16_t pdpt_end;
  uint16_t pd_start;
  uint16_t pd_end;
  uint16_t pt_start;
  uint16_t pt_end;
  unsigned char avail; // availability flag
}map_ledger_entry;

map_ledger_entry* g_map_ledger;

// dynamic virtual address base
//                    0x  80000000 2 GiB
//                    0x  40000000 1 GiB
uint64_t g_dyn_base = 0x8000000000;

// page sizes in hexadecimal
// 1 GiB = 0x40000000
// 2 MiB = 0x200000
// 4 KiB = 0x1000


// 1 GiB is 512 2 MiB pages

/**
 * Creates a PML4E to hold the address of a PDPT.
 * The resulting PML4E is marked as present, read/write, and supervisor mode.
 * The memory cache type bits, PKE bits, and execute disable bit are all
 * left as 0.
 *
 *
 * Params:
 *   uint64_t - the address of a PDPT
 *
 * Returns:
 *   pml4e - a PML4 entry
 */
pml4e make_pml4e(pdpte* pdpt_addr)
{
  pml4e p = 0;

  // Bit 0 is the present flag.
  // Mark the page as present.
  p |= BM_0;

  // Bit 1 is the read/write flag.
  // Set this entry to be read/write.
  p |= BM_1;

  // Bit 2 is the user/supvisor mode flag.
  // Leaving it at 0 since there's no user mode yet.

  // Bit 3 is the PWT bit.
  // Bit 4 is the PCD bit.
  // Bit 5 is the access bit.
  // Bit 6 is ignored.
  // Bit 7 is reserved, and must be 0.
  // Bits [11:8] are ignored.

  // Bits [51:12] are the address of the PDPT entry.
  p |= (uint64_t)pdpt_addr;

  // Bits [62:52] are ignored

  // Bit 63 is the execute-disable bit.
  // It's only used if IA32_EFER.NXE = 1, but we'll
  // leave it as 0 here.

  return p;
}


/**
 * Creates a PDPT entry.
 * The resulting PDPTE is marked as present, read/write, and supervisor mode.
 * The memory cache type bits, PKE bits, and execute disable bit are all
 * left as 0.
 *
 * Params:
 *   pde* - base address of a page directory
 *
 * Returns:
 *   pdpte - a PDPT entry
 */
pdpte make_pdpte(pde* pd_addr)
{
  pdpte p = 0;
  uint64_t bit_mask = 0x1; // single bit mask

  // Bit 0 is the present flag.
  // Mark the page as present.
  p |= BM_0;

  // Bit 1 is the read/write flag.
  // Set this entry to be read/write.
  p |= BM_1;

  // Bit 2 is the user/supvisor mode flag.
  // Leaving it at 0 since there's no user mode yet.

  // Bit 3 is the PWT bit.
  // Bit 4 is the PCD bit.
  // Bit 5 is the access flag.
  // Bit 6 is ignored.
  // Bit 7 is the page size bit and must be 0 in order
  // to point to a page directory.

  // Bits [11:8] are ignored.

  // Bits [51:12] are the base address of a page directory.
  p |= (uint64_t)pd_addr;

  // Bits [62:52] are ignored

  // bit 63 is the execute-disable bit.
  // It's only used if IA32_EFER.NXE = 1, but we'll
  // leave it as 0 here.

  return p;
}

/**
 * Creates a PDE.
 * The resulting PDE is marked as present, read/write, and supervisor mode.
 * The memory cache type bits, PKE bits, and execute disable bit are all
 * left as 0.
 *
 * Params:
 *   pte* - base address of a page table
 *
 * Returns:
 *   pde - a page directory entry pointing to a page table
 */
pde make_pde(pte* pt_addr)
{
  pde p = 0;

  // Bit 0 is the present flag.
  // Mark the page as present.
  p |= BM_0;

  // Bit 1 is the read/write flag.
  // Set this entry to be read/write.
  p |= BM_1;

  // Bit 2 is the user/supvisor mode flag.
  // Leaving it at 0 since there's no user mode yet.

  // Bit 3 is the PWT bit.
  // Bit 4 is the PCD bit.
  // Bit 5 is the access flag.
  // Bit 6 is ignored.
  // Bit 7 is the page size bit and must be 0 in order
  // to point to a page directory.

  // Bits [11:8] are ignored.

  // Bits [51:12] are the base address of a page table.
  p |= (uint64_t)pt_addr;

  // Bits [62:52] are ignored

  // bit 63 is the execute-disable bit.
  // It's only used if IA32_EFER.NXE = 1, but we'll
  // leave it as 0 here.

  return p;
}

/**
 * Creates a PTE.
 * The resulting PTE is marked as present, read/write, and supervisor mode.
 * The memory cache type bits, PKE bits, and execute disable bit are all
 * left as 0.
 * The base address of the page must be 4 KiB aligned.
 *
 * Params:
 *   uint64_t - the base address of a 4 KiB page
 *
 * Returns:
 *   pte - a page table entry
 */
pte make_pte(uint64_t page_addr)
{
  pte p = 0;

  // Bit 0 is the present flag.
  // Mark the page as present.
  p |= BM_0;

  // Bit 1 is the read/write flag.
  // Set this entry to be read/write.
  p |= BM_1;

  // Bit 2 is the user/supvisor mode flag.
  // Leaving it at 0 since there's no user mode yet.

  // Bit 3 is the PWT bit.
  // Bit 4 is the PCD bit.
  // Bit 5 is the access flag.
  // Bit 6 is ignored.
  // Bit 7 is the PAT bit.
  // Bit 8 is the global translation flag. Leaving it as 0 for now.

  // Bits [11:9] are ignored.

  // Bits [51:12] are the address of the page.
  p |= page_addr;

  // Bits [58:52] are ignored.

  // Bits [62:59] are the protection key.
  // This is only applicable if CR4.PKE = 1.

  // bit 63 is the execute-disable bit.
  // It's only used if IA32_EFER.NXE = 1, but we'll
  // leave it as 0 here.

  return p;
}


void k_paging_init()
{
  // Fail if the system RAM is >= 512 GiB.
  // This is to prevent a possible encroachment into the dynamic
  // virtual address mapping space.
  if (g_total_ram >= 0x8000000000)
  {
    fprintf(stddbg, "[ERROR] RAM size too large.\n");
    for (;;);
  }

  // Calculate the paging structures required to map all of RAM.
  uint64_t ram_counter = 0;
  uint64_t ptn = 0;   // number of page tables
  uint64_t pdn = 1;   // number of page directories
  uint64_t pdptn = 1; // number of page directory pointer tables
  while (ram_counter < g_total_ram)
  {
    if (!(ram_counter % 0x200000))
    {
      // If the counter reaches a multiple of 2 MiB,
      // increment the number of page tables.
      ptn++;

      // If the number of page tables reaches a multiple
      // of 512, increment the number of page directories.
      if (!(ptn % 512))
      {
        pdn++;
      }

      // If the number of page directories reaches a multiple
      // of 512, increment the number of PDPTs.
      if (!(pdn % 512))
      {
        pdptn++;
      }
    }
    ram_counter += 0x1000;
  }

  // If we have an even multiple of 512 page directories,
  // then we preemptively incremented the PDPT count, so
  // we correct that here.
  if (!(pdn % 512))
  {
    pdptn--;
  }

  // Correct the page directory count like we did with the
  // PDPT count
  if (!(ptn % 512))
  {
    pdn--;
  }

  // Get memory for the page tables.
  g_pt_mem = (pte*)k_memory_alloc_pages(ptn);
  if (g_pt_mem == NULL)
  {
    fprintf(stddbg, "[ERROR] failed to allocate memory for page tables\n");
    for (;;);
  }

  // Get memory for the page page directories.
  g_pd_mem = (pde*)k_memory_alloc_pages(pdn);
  if (g_pd_mem == NULL)
  {
    fprintf(stddbg, "[ERROR] failed to allocate memory for page directories\n");
    for (;;);
  }

  // Get memory for the PDPTs.
  g_pdpt_mem = (pdpte*)k_memory_alloc_pages(pdptn);
  if (g_pdpt_mem == NULL)
  {
    fprintf(stddbg, "[ERROR] failed to allocate memory for PDPTs\n");
    for (;;);
  }

  // Get memory for a PML4
  g_pml4_mem = (pml4e*)k_memory_alloc_pages(1);
  if (g_pml4_mem == NULL)
  {
    fprintf(stddbg, "[ERROR] failed to allocate memory for PML4\n");
    for (;;);
  }

  // Fully populate each page table to identity map RAM.
  // Every page table should be populated with present pages.
  uint64_t addr = 0;
  for (int i = 0; i < ptn * 512; i++)
  {
    g_pt_mem[i] = make_pte(addr);
    addr += 0x1000;
  }

  // Fill each page directory with the addresses of our page tables.
  // Every page directory should be populated with present page tables.
  for (int i = 0; i < pdn; i++)
  {
    for (int j = 0; j < 512; j++)
    {
      uint64_t pd_offset = 512 * i + j;
      uint64_t pt_offset = 512 * pd_offset;

      g_pd_mem[pd_offset] = make_pde(&g_pt_mem[pt_offset]);
    }
  }

  // Fill each PDPT with the addresses of our page directories.
  // Once all of our page directories are entered into the PDPTs,
  // we fill the remaining slots in the PDPTs with non present PDPTEs.
  for (int i = 0; i < pdptn; i++)
  {
    for (int j = 0; j < 512; j++)
    {
      uint64_t pdpt_offset = 512 * i + j;

      // If we've already entered all of our page directories into
      // the PDPTs, start using 0 as the offset for the PD memory
      // since we're going to mark the PDPTE as not present anyway.
      uint64_t pd_offset = pdpt_offset < pdn ? 512 * pdpt_offset : 0;

      g_pdpt_mem[pdpt_offset] = make_pdpte(&g_pd_mem[pd_offset]);

      // If we've already entered all of our page directories into
      // the PDPTs, start marking the remaining PDPTEs as not present.
      if (pdpt_offset >= pdn)
      {
        g_pdpt_mem[pdpt_offset] &= ~BM_0;
      }
    }
  }

  // Fill the PML4 with the addresses of our PDPTs.
  // Once all of our PSPTs are entered into the PML4,
  // we fill the remaining slots in the PML4 with non present PML4Es.
  for (int i = 0; i < 512; i++)
  {
    // If we've already entered all of our PDPTs into
    // the PML4, start using 0 as the offset for the PDPT memory
    // since we're going to mark the PML4E as not present anyway.
    uint64_t pdpt_offset = i < pdptn ? 512 * i : 0;

    g_pml4_mem[i] = make_pml4e(&g_pdpt_mem[pdpt_offset]);

    // If we've already entered all of our PDPTs into
    // the PML4, start marking the remaining PDPTEs as not present.
    if (i >= pdptn)
    {
      g_pml4_mem[i] &= ~BM_0;
    }
  }

  // Update CR3 with the address of the PML4.
  k_set_cr3((uint64_t)g_pml4_mem);


  fprintf(stddbg, "sizeof map_ledger_entry: %llu\n", sizeof(map_ledger_entry));

  // Create the virtual address map ledger
  g_map_ledger = (map_ledger_entry*)k_memory_alloc_pages(8);
  if (g_map_ledger == NULL)
  {
    fprintf(stddbg, "[ERROR] failed to allocate memory for virtual map ledger\n");
    for (;;);
  }

  // Mark all ledger entries as available.
  for (int i = 0; i < MAP_LEDGER_MAX; i++)
  {
    g_map_ledger[i].avail = 1;
  }
}


// A PML4 rentry maps 512 GiB.
// A PDPT entry maps 1 GiB.
// A PD entry maps 2 MiB.
// A PT entry maps 4 KiB.
// 
// so PML4[1] can map the range from 0x8000000000 to 0xFFFFFFFFFF
// using one PDPT, 512 PDs, and 262,144 PTs.
// 


uint64_t k_paging_map_range(uint64_t start, uint64_t end)
{
  // physical addresses
  uint64_t phys_start = start;
  uint64_t phys_end = end;

  // virtual addresses
  uint64_t virt_start;
  uint64_t virt_end;

  // Round the starting and ending addresses down to the nearest 4 KiB
  // boundary.
  for (;phys_start % 0x1000; phys_start--);
  for (;phys_end % 0x1000; phys_end--);

  // Determine the size of the range.
  uint64_t size = phys_end - phys_start;

  fprintf(stddbg, "[PAGING] phys start: %llX, end: %llX, size: %llu\n", phys_start, phys_end, size);

  // Determine the virtual address range
  virt_start = g_dyn_base;
  virt_end = virt_start + size;

  fprintf(stddbg, "[PAGING] virt start: %llX, end: %llX, size: %llu\n", virt_start, virt_end, size);

  // Create a new entry in the ledger
  for (int i = 0, entry = 0; i < MAP_LEDGER_MAX && !entry; i++)
  {
    if (g_map_ledger[i].avail)
    {
      g_map_ledger[i].avail = 0;
      g_map_ledger[i].start = virt_start;
      g_map_ledger[i].end = virt_end;
      entry = 1;
    }
  }

  // Determine the paging structures needed to map the range.
  uint64_t ram_counter = 0;
  uint64_t ptn = 0;   // number of page tables
  uint64_t pdn = 1;   // number of page directories
  uint64_t pdptn = 1; // number of page directory pointer tables
  while (ram_counter < size)
  {
    if (!(ram_counter % 0x200000))
    {
      // If the counter reaches a multiple of 2 MiB,
      // increment the number of page tables.
      ptn++;

      // If the number of page tables reaches a multiple
      // of 512, increment the number of page directories.
      if (!(ptn % 512))
      {
        pdn++;
      }

      // If the number of page directories reaches a multiple
      // of 512, increment the number of PDPTs.
      if (!(pdn % 512))
      {
        pdptn++;
      }
    }
    ram_counter += 0x1000;
  }

  // If we have an even multiple of 512 page directories,
  // then we preemptively incremented the PDPT count, so
  // we correct that here.
  if (!(pdn % 512))
  {
    pdptn--;
  }

  // Correct the page directory count like we did with the
  // PDPT count
  if (!(ptn % 512))
  {
    pdn--;
  }


  fprintf(stddbg, "mapping %llu bytes would require %llu PTs, %llu PDs, and %llu PDPTs\n", size, ptn, pdn, pdptn);


  // A virtual address is 48 bits with the following structure:
  // 
  //    [47:39]      [38:30]     [29:21]    [20:12]     [11:0]
  // +------------+------------+----------+----------+------------+
  // | PML4 index | PDPT index | PD index | PT index |   offset   |
  // +------------+------------+----------+----------+------------+
  //   9 bits       9 bits       9 bits     9 bits       12 bits


  // Break the starting and ending virtual addresses into their
  // paging structure index components.
  uint64_t pml4_start = (virt_start & (BM_9_BITS << 39)) >> 39;
  uint64_t pdpt_start = (virt_start & (BM_9_BITS << 30)) >> 30;
  uint64_t pd_start = (virt_start & (BM_9_BITS << 21)) >> 21;
  uint64_t pt_start = (virt_start & (BM_9_BITS << 12)) >> 12;
  uint64_t pt_offset = virt_start & BM_12_BITS;

  uint64_t pml4_end = (virt_end & (BM_9_BITS << 39)) >> 39;
  uint64_t pdpt_end = (virt_end & (BM_9_BITS << 30)) >> 30;
  uint64_t pd_end = (virt_end & (BM_9_BITS << 21)) >> 21;
  uint64_t pt_end = (virt_end & (BM_9_BITS << 12)) >> 12;


  // Currently, we don't allow mappings that pass 0xFFFFFFFFFF
  if (pml4_end > 1)
  {
    return 0;
  }

  fprintf(stddbg,
    "virtual start: %llX\nPML4: %llu, PDPT: %llu, PD: %llu, PT: %llu offset: %llu\n",
    virt_start,
    pml4_start,
    pdpt_start,
    pd_start,
    pt_start,
    pt_offset
  );

  fprintf(stddbg,
    "virtual end:   %llX\nPML4: %llu, PDPT: %llu, PD: %llu, PT: %llu offset: %llu\n",
    virt_end,
    (virt_end & (BM_9_BITS << 39)) >> 39,
    (virt_end & (BM_9_BITS << 30)) >> 30,
    (virt_end & (BM_9_BITS << 21)) >> 21,
    (virt_end & (BM_9_BITS << 12)) >> 12,
    virt_end & BM_12_BITS
  );
}
