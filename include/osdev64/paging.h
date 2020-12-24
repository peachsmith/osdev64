#ifndef JEP_PAGING_H
#define JEP_PAGING_H

#include <stdint.h>

/**
 * Initializes paging.
 * This function overrides any paging established by the firmware.
 */
void k_paging_init();


/**
 * A PML4 entry is 64 bits with the following structure:
 * ---------------------------------------------------------------------------
 * bits      purpose
 * ---------------------------------------------------------------------------
 * 0         present (must be 1 to reference a PDPT)
 * 1         read/write (0 for read only, 1 for read/write)
 * 2         user/supervisor (0 if user-mode access is not allowed)
 * 3         page-level write through
 * 4         page-level cache disabled
 * 5         access (indicates whether this entry has been used for translation)
 * 6         ignored
 * 7         reserved (must be 0)
 * 8 - 11    ignored
 * 12 - 50   physical address of PDPT
 * 51        reserved (must be 0)
 * 52 - 62   ignored
 * 63        execute-disable (if 1, instruction fetches not allowed). This
 *           only applied if IA32_EFER.NXE = 1, otherwise this bit should be 0
 * ---------------------------------------------------------------------------
 */
typedef uint64_t pml4e;


/**
 * A PDPT entry is 64 bits with the following structure:
 * ---------------------------------------------------------------------------
 * bits      purpose
 * ---------------------------------------------------------------------------
 * 0         present (must be 1 to reference a PDPT)
 * 1         read/write (0 for read only, 1 for read/write)
 * 2         user/supervisor (0 if user-mode access is not allowed)
 * 3         page-level write through
 * 4         page-level cache disabled
 * 5         access (indicates whether this entry has been used for translation)
 * 6         ignored
 * 7         reserved (must be 0)
 * 8 - 11    ignored
 * 12 - 50   physical address of PDPT
 * 51        reserved (must be 0)
 * 52 - 62   ignored
 * 63        execute-disable (if 1, instruction fetches not allowed). This
 *           only applied if IA32_EFER.NXE = 1, otherwise this bit should be 0
 * ---------------------------------------------------------------------------
 */
typedef uint64_t pdpte;

#endif