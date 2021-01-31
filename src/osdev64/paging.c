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

// typedef struct map_ledger_entry {
//   uint64_t start;
//   uint64_t end;
//   uint16_t pdpt_start;
//   uint16_t pdpt_end;
//   uint16_t pd_start;
//   uint16_t pd_end;
//   uint16_t pt_start;
//   uint16_t pt_end;
//   unsigned char avail; // availability flag
// }map_ledger_entry;

/**
 * A single entry in the virtual address mapping ledger.
 */
typedef struct map_ledger_entry {
  k_regn start;
  k_regn end;
  unsigned char avail; // availability flag
}map_ledger_entry;

map_ledger_entry* g_map_ledger;


// dynamic virtual address base
//                    0x  80000000 2 GiB
//                    0x  40000000 1 GiB
uint64_t g_dyn_base = 0x8000000000;


// PDPT used for dynamic mapping
pdpte* g_dyn_pdpt;

// bitmap for tracking page directories for dynamic mapping
uint64_t g_dyn_pd_map[8];

// bitmap for tracking page directories for dynamic mapping
uint64_t* g_dyn_pt_map;


// NOTE:
// page sizes in hexadecimal
// 1 GiB = 0x40000000
// 2 MiB = 0x200000
// 4 KiB = 0x1000

// NOTE:
// 1 GiB is 512 2 MiB pages


uint64_t get_bit(uint64_t* map, uint64_t bit)
{
  uint64_t i = bit / 64; // which index in the array
  uint64_t b = bit % 64; // which bit in the uint64_t

  return (map[i] & ((uint64_t)1 << b));
}

uint64_t set_bit(uint64_t* map, uint64_t bit, int val)
{
  uint64_t i = bit / 64; // which index in the array
  uint64_t b = bit % 64; // which bit in the uint64_t

  if (val)
  {
    map[i] |= ((uint64_t)1 << b);
  }
  else
  {
    map[i] &= ~((uint64_t)1 << b);
  }
}

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
static inline pml4e make_pml4e(pdpte* pdpt_addr)
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
static inline pdpte make_pdpte(pde* pd_addr)
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
static inline pde make_pde(pte* pt_addr)
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
static inline pte make_pte(uint64_t page_addr)
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
    HANG();
  }


  //=============================================
  // BEGIN static mapping initialization

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
    HANG();
  }

  // Get memory for the page page directories.
  g_pd_mem = (pde*)k_memory_alloc_pages(pdn);
  if (g_pd_mem == NULL)
  {
    fprintf(stddbg, "[ERROR] failed to allocate memory for page directories\n");
    HANG();
  }

  // Get memory for the PDPTs.
  g_pdpt_mem = (pdpte*)k_memory_alloc_pages(pdptn);
  if (g_pdpt_mem == NULL)
  {
    fprintf(stddbg, "[ERROR] failed to allocate memory for PDPTs\n");
    HANG();
  }

  // Get memory for a PML4
  g_pml4_mem = (pml4e*)k_memory_alloc_pages(1);
  if (g_pml4_mem == NULL)
  {
    fprintf(stddbg, "[ERROR] failed to allocate memory for PML4\n");
    HANG();
  }

  // Fill the page tables with base addresses of pages.
  k_regn addr = 0;
  for (int i = 0; i < ptn * 512; i++)
  {
    g_pt_mem[i] = make_pte(addr);
    addr += 0x1000;
  }

  // Fill the page directories with addresses of page tables.
  for (int i = 0; i < pdn; i++)
  {
    for (int j = 0; j < 512; j++)
    {
      uint64_t pd_offset = 512 * i + j;

      if (pd_offset < ptn)
      {
        g_pd_mem[pd_offset] = make_pde(&g_pt_mem[512 * pd_offset]);
      }
      else
      {
        g_pd_mem[pd_offset] = 0;
      }
    }
  }

  // Fill the PDPTs with addresses of page directories.
  for (int i = 0; i < pdptn; i++)
  {
    for (int j = 0; j < 512; j++)
    {
      uint64_t pdpt_offset = 512 * i + j;

      if (pdpt_offset < pdn)
      {
        g_pdpt_mem[pdpt_offset] = make_pdpte(&g_pd_mem[512 * pdpt_offset]);
      }
      else
      {
        g_pdpt_mem[pdpt_offset] = 0;
      }
    }
  }

  // Fill the PML4 with addresses of PDPTs.
  for (int i = 0; i < 512; i++)
  {
    if (i < pdptn)
    {
      g_pml4_mem[i] = make_pml4e(&g_pdpt_mem[512 * i]);
    }
    else
    {
      g_pml4_mem[i] = 0;
    }
  }

  // Update CR3 with the address of the PML4.
  k_set_cr3(PTR_TO_N(g_pml4_mem));

  // END static mapping initialization
  //=============================================


  //=============================================
  // BEGIN dynamic mapping initialization


  // Create the virtual address map ledger
  g_map_ledger = (map_ledger_entry*)k_memory_alloc_pages(8);
  if (g_map_ledger == NULL)
  {
    fprintf(stddbg, "[ERROR] failed to allocate memory for virtual map ledger\n");
    HANG();
  }

  // Mark all ledger entries as available.
  for (int i = 0; i < MAP_LEDGER_MAX; i++)
  {
    g_map_ledger[i].avail = 1;
  }


  // Get memory for the dynamic page table bitmap
  g_dyn_pt_map = (uint64_t*)k_memory_alloc_pages(8);
  if (g_dyn_pt_map == NULL)
  {
    fprintf(stddbg, "[ERROR] failed to allocate memory for dynamic page table bitmap.\n");
    HANG();
  }

  // Set all the entries in the dynamic page table bitmap to 0.
  for (int i = 0; i < 4096; i++)
  {
    g_dyn_pt_map[i] = 0;
  }

  // Set all the entries in the dynamic page directory bitmap to 0.
  for (int i = 0; i < 8; i++)
  {
    g_dyn_pd_map[i] = 0;
  }


  // Create the PDPT for dynamic mapping
  g_dyn_pdpt = (pdpte*)k_memory_alloc_pages(1);
  if (g_dyn_pdpt == NULL)
  {
    fprintf(stddbg, "failed to allocate memory for PDPT for dynamic mapping\n");
    HANG();
  }

  // Set all entries in the dynamic PDPT to 0.
  for (int i = 0; i < 512; i++)
  {
    g_dyn_pdpt[i] = 0;
  }

  // Put the dynamic PDPT in the PML4.
  g_pml4_mem[1] = make_pml4e(g_dyn_pdpt);


  // END dynamic mapping initialization
  //=============================================
}


// A PML4 rentry maps 512 GiB.
// A PDPT entry maps 1 GiB.
// A PD entry maps 2 MiB.
// A PT entry maps 4 KiB.
// 
// so PML4[1] can map the range from 0x8000000000 to 0xFFFFFFFFFF
// using one PDPT, 512 PDs, and 262,144 PTs.
// 


k_regn k_paging_map_range(k_regn start, k_regn end)
{
  // Immediately fail if the start address is higher
  // than the end address.
  if (start > end)
  {
    return 0;
  }

  // physical addresses
  k_regn phys_start = start;
  k_regn phys_end = end;

  // virtual addresses
  k_regn virt_start;
  k_regn virt_end;

  // virtual address offset
  k_regn virt_offset = 0;

  // Round the starting and ending addresses down to the nearest 4 KiB
  // boundary.
  for (;phys_start % 0x1000; phys_start--) virt_offset++;
  for (;phys_end % 0x1000; phys_end--);

  // Determine the size of the range.
  k_regn size = phys_end - phys_start;


  // Initialize the start of the virtual address range to the
  // global dynamic virtual address base.
  virt_start = g_dyn_base;
  virt_end = virt_start + size;

  // Check the ledger to see if there are existing virtual address
  // ranges that were previously mapped but are now available.
  for (int i = 0; i < MAP_LEDGER_MAX; i++)
  {
    if (!g_map_ledger[i].avail)
    {
      // start and end addresses of reserved memory region
      k_regn res_start = g_map_ledger[i].start;
      k_regn res_end = g_map_ledger[i].end;

      // If the requested region overlaps a reserved region,
      // update the start and end addresses of the requested region.
      if (
        (virt_start >= res_start && virt_start <= res_end)
        ||
        (res_start >= virt_start && res_start <= virt_end)
        )
      {
        virt_start = res_end + 0x1000;
        virt_end = virt_start + size;
      }
    }
  }

  // Create a new virtual address reservation.
  for (int i = 0, entry = 0; i < RAM_LEDGER_MAX && !entry; i++)
  {
    if (g_map_ledger[i].avail)
    {
      g_map_ledger[i].avail = 0;
      g_map_ledger[i].start = virt_start;
      g_map_ledger[i].end = virt_end;
      entry = 1;
    }
  }



  //=====================================
  // BEGIN paging structure population

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


  // A virtual address is 48 bits with the following structure:
  // 
  //    [47:39]      [38:30]     [29:21]    [20:12]     [11:0]
  // +------------+------------+----------+----------+------------+
  // | PML4 index | PDPT index | PD index | PT index |   offset   |
  // +------------+------------+----------+----------+------------+
  //   9 bits       9 bits       9 bits     9 bits       12 bits


  // Break the starting and ending virtual addresses into their
  // paging structure index components.
  k_regn pml4_start = (virt_start & (BM_9_BITS << 39)) >> 39;
  k_regn pdpt_start = (virt_start & (BM_9_BITS << 30)) >> 30;
  k_regn pd_start = (virt_start & (BM_9_BITS << 21)) >> 21;
  k_regn pt_start = (virt_start & (BM_9_BITS << 12)) >> 12;
  k_regn pt_offset = virt_start & BM_12_BITS;

  k_regn pml4_end = (virt_end & (BM_9_BITS << 39)) >> 39;
  k_regn pdpt_end = (virt_end & (BM_9_BITS << 30)) >> 30;
  k_regn pd_end = (virt_end & (BM_9_BITS << 21)) >> 21;
  k_regn pt_end = (virt_end & (BM_9_BITS << 12)) >> 12;


  // Currently, we don't allow mappings that pass 0xFFFFFFFFFF
  if (pml4_end > 1)
  {
    return 0;
  }

  k_regn addr = phys_start;

  // Allocate page directories
  for (uint64_t i = pdpt_start; i <= pdpt_end; i++)
  {
    // Ensure that the page directory exists.
    if (!get_bit(g_dyn_pd_map, i))
    {
      fprintf(stddbg, "[DEBUG] creating new page directory\n");

      pde* new_dir = (pde*)k_memory_alloc_pages(1);
      if (new_dir == NULL)
      {
        fprintf(stddbg, "failed to allocate memory for dynamic page directory\n");
        return 0;
      }

      g_dyn_pdpt[i] = make_pdpte(new_dir);

      set_bit(g_dyn_pd_map, i, 1);

      // Allocate page tables
      pte* new_tabs = (pte*)k_memory_alloc_pages(512);
      if (new_tabs == NULL)
      {
        fprintf(stddbg, "failed to allocate memory for dynamic page tables\n");
        return 0;
      }

      // Fill the new page directory.
      for (int j = 0; j < 512; j++)
      {
        for (int k = 0; k < 512; k++)
        {
          new_tabs[512 * j + k] = 0;
        }

        new_dir[j] = make_pde(&new_tabs[512 * j]);
      }
    }

    // Extract the directory address from the PDPTE.
    pde* dir = (pde*)(g_dyn_pdpt[i] & (BM_40_BITS << 12));

    // Populate the page directory.
    for (uint64_t j = pd_start; j <= (i < pdpt_end ? 511 : pd_end); j++)
    {
      // Extract the table address from the directory.
      pte* tab = (pte*)(dir[j] & (BM_40_BITS << 12));

      // Populate the page table.
      for (uint64_t k = pt_start; k <= (i < pdpt_end || j < pd_end ? 511 : pt_end); k++)
      {
        if (addr <= phys_end)
        {
          // for debugging
          // fprintf(stddbg,
          //   "[DEBUG] mapping physical %p to virtual: %p\n",
          //   addr,
          //   (((uint64_t)1 << 39) | (i << 30) | (j << 21) | (k << 12))
          // );

          tab[k] = make_pte(addr);
          addr += 0x1000;
        }
      }
    }
  }

  // END paging structure population
  //=====================================

  // Update the global dynamic mapping base address.
  g_dyn_base = virt_end + 0x1000;

  return virt_start + virt_offset;
}


void k_paging_unmap_range(k_regn start)
{
  for (int i = 0; i < MAP_LEDGER_MAX; i++)
  {
    // Find the first entry in the virtual address ledger
    // whose starting address matches the specified address
    // and mark it as available.
    if (g_map_ledger[i].start == start && !g_map_ledger[i].avail)
    {
      g_map_ledger[i].avail = 1;
      return;
    }
  }
}


void k_paging_print_ledger()
{
  fprintf(stddbg, "+------------------------------------+\n");
  fprintf(stddbg, "| start             end              |\n");
  fprintf(stddbg, "+------------------------------------+\n");

  for (int i = 0; i < MAP_LEDGER_MAX; i++)
  {
    if (!g_map_ledger[i].avail)
    {
      fprintf(stddbg,
        "| %p  %p |\n",
        g_map_ledger[i].start,
        g_map_ledger[i].end
      );
    }
  }

  fprintf(stddbg, "+------------------------------------+\n");
}
