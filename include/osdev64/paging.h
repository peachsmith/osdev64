#ifndef JEP_PAGING_H
#define JEP_PAGING_H

#include "osdev64/axiom.h"


/**
 * Paging interface.
 * Functions and data types for handling virtual memory.
 *
 *
 * Four Fundamental Paging Structures:
 *
 *   Page Table (PT):
 *     a list of addresses of 4KiB pages
 *
 *   Page Directory (PD):
 *     a list of addresses of page tables
 *
 *   Page Directory Pointer Table (PDPT)
 *     a list of addresses of page directories
 *
 *   Page Map Level 4 (PML4):
 *     a list of addresses of PDPTs.
 *
 * Each paging structure is 4096 bytes. So in 64-bit mode, where
 * each address is a 64-bit number, each paging structure will have
 * 512 entries.
 *
 *
 * Memory Cache Type Bits:
 *
 *   Page-Level Write Through   (PWT)
 *   Page-Level Cache Disable   (PCD)
 *   Page Attribute Aable       (PAT)
 *
 * The PWT, PCD, and PAT bits in a paging structure effectively form the
 * index in the PAT. This is used to indicate memory caching modes of
 * virtual addresses.
 * Bits 3 and 4 are the PWT and PCD bits respectively of any given paging
 * structure. For paging structures that point to pages larger than 4 KiB,
 * bit 12 is the PAT bit. For paging structures that point to 4 KiB pages,
 * bit 7 is the PAT bit.
 * Paging structures that point to other paging structures do not have a
 * PAT bit.
 *
 * Stuff I haven't bothered to research:
 *   Process Context Identifier (PCIDE)
 *   Protection Key             (PKE)
 *
 */

;


/**
 * Page Map Level 4 Entry (PML4E)
 * A PML4E that points to a PDPT is 64 bits with the following structure:
 * ---------------------------------------------------------------------------
 * bits      purpose
 * ---------------------------------------------------------------------------
 * 0       present flag
 * 1       read/write (0 for read only, 1 for read/write)
 * 2       user/supervisor (0 if user-mode access is not allowed)
 * 3       PWT
 * 4       PCD
 * 5       access (indicates whether this entry has been used for translation)
 * 6       ignored
 * 7       reserved (must be 0)
 * [11:8]  ignored
 * [51:12] physical address of PDPT (4KiB aligned)
 * [62:52] ignored
 * 63      execute-disable (only applied if IA32_EFER.NXE = 1)
 * ---------------------------------------------------------------------------
 */
typedef k_regn pml4e;


/**
 * Page Directory Pointer Table Entry (PDPTE)
 * A PDPTE that points to a PD has the following structure:
 * ---------------------------------------------------------------------------
 * bits      purpose
 * ---------------------------------------------------------------------------
 * 0       present flag
 * 1       read/write (0 for read only, 1 for read/write)
 * 2       user/supervisor (0 if user-mode access is not allowed)
 * 3       PWT
 * 4       PCD
 * 5       access flag
 * 6       ignored
 * 7       page size (must be 1 if pointing to a 1 GiB page)
 * [11:8]  ignored
 * [51:12] physical address of a page directory (4KiB aligned)
 * [62:52] ignored
 * 63      execute disable (only applicable if IA32_EFER.NXE = 1)
 * ---------------------------------------------------------------------------
 */
typedef k_regn pdpte;

/**
 * Page Directory Entry (PDE)
 * A PDE that points to a PT has the following structure:
 * ---------------------------------------------------------------------------
 * bits      purpose
 * ---------------------------------------------------------------------------
 * 0       present flag
 * 1       read/write (0 for read only, 1 for read/write)
 * 2       user/supervisor (0 if user-mode access is not allowed)
 * 3       PWT
 * 4       PCD
 * 5       access flag
 * 6       ignored
 * 7       page size (must be 1 if pointing to a 2 MiB page)
 * [11:8]  ignored
 * [51:12] address of page table (4KiB aligned)
 * [29:13] reserved (must be 0)
 * [51:30] physical address of a page table
 * [62:52] ignored
 * 63      execute disable (only applicable if IA32_EFER.NXE = 1)
 * ---------------------------------------------------------------------------
 */
typedef k_regn pde;

/**
 * Page Table Entry (PTE)
 * A PTE that points to a 4 KiB page has the following structure:
 * ---------------------------------------------------------------------------
 * bits      purpose
 * ---------------------------------------------------------------------------
 * 0       present flag
 * 1       read/write (0 for read only, 1 for read/write)
 * 2       user/supervisor (0 if user-mode access is not allowed)
 * 3       PWT
 * 4       PCD
 * 5       access flag
 * 6       dirty flag
 * 7       PAT
 * [11:8]  ignored
 * [51:12] address of 4 KiB page (4KiB aligned)
 * [58:52] ignored
 * [62:59] protection key (only applicable if CR4.PKE = 1)
 * 63      execute disable (only applicable if IA32_EFER.NXE = 1)
 * ---------------------------------------------------------------------------
 */
typedef k_regn pte;


/**
 * Initializes paging interface.
 * This must be called before any other functions in this interface.
 */
void k_paging_init();

/**
 * Maps a range of physical addresses to a range of virtual addresses.
 * The two arguments specify the lowest and highest addresses in the
 * physical address range to be mapped.
 * Upon success, this function returns the base address of a virtual
 * address range.
 * On failure, 0 is returned.
 *
 * Params:
 *   k_regn - the lowest address in the range of physical addresses
 *   k_regn - the highest address in the range of physical addresses
 *
 * Returns:
 *   k_regn - the base address of the range of virtual addresses
 *
 */
k_regn k_paging_map_range(k_regn start, k_regn end);


/**
 * Releases a virtual address mapping starting at the specified address.
 *
 * Params:
 *   k_regn - the base address of a range of virtual addresses
 */
void k_paging_unmap_range(k_regn start);

/**
 * This function writes the contents of the dynamic virtual address ledger
 * to some output stream. It is intended to be used for debugging.
 * The actual output destination is undefined.
 */
void k_paging_print_ledger();


#endif