#ifndef JEP_PAGING_H
#define JEP_PAGING_H

#include <stdint.h>


/**
 * Paging interface.
 * 
 * Some paging concepts:
 *   page-level write through   (PWT)
 *   page-level Cache Disable   (PCD)
 *   page attribute table       (PAT)
 *   process context identifier (PCIDE)
 *   protection key             (PKE)
 * 
 */


/**
 * Initializes paging.
 * This function overrides any paging established by the firmware.
 */
void k_paging_init();


/**
 * A PML4 entry that points to a PDPT is 64 bits with the following structure:
 * ---------------------------------------------------------------------------
 * bits      purpose
 * ---------------------------------------------------------------------------
 * 0       present flag
 * 1       read/write (0 for read only, 1 for read/write)
 * 2       user/supervisor (0 if user-mode access is not allowed)
 * 3       page-level write through
 * 4       page-level cache disabled
 * 5       access (indicates whether this entry has been used for translation)
 * 6       ignored
 * 7       reserved (must be 0)
 * [11:8]  ignored
 * [51:12] physical address of PDPT
 * [62:52] ignored
 * 63      execute-disable (only applied if IA32_EFER.NXE = 1)
 * ---------------------------------------------------------------------------
 */
typedef uint64_t pml4e;


/**
 * A PDPT entry that points to a 1 GiB page is 64 bits with the following
 * structure:
 * ---------------------------------------------------------------------------
 * bits      purpose
 * ---------------------------------------------------------------------------
 * 0       present flag
 * 1       read/write (0 for read only, 1 for read/write)
 * 2       user/supervisor (0 if user-mode access is not allowed)
 * 3       page-level write through
 * 4       page-level cache disabled
 * 5       access flag
 * 6       dirty flag
 * 7       page size (must be 1 in order to point to a 1 GiB page)
 * 8       global flag (only applicable if  CR4.PGE = 1)
 * [11:9]  ignored
 * 12      page attribute table (PAT) flag
 * [29:13] reserved (must be 0)
 * [51:30] physical address of 1 GiB page
 * [58:52] ignored
 * [62:59] protection key (only applicable if CR4.PKE = 1)
 * 63      execute disable (only applicable if IA32_EFER.NXE = 1)
 * ---------------------------------------------------------------------------
 */
typedef uint64_t pdpte;

#endif