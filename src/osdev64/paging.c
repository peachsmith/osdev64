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

static char* mem_Uncacheable = "UC";
static char* mem_WriteCombining = "WC";
static char* mem_WriteThrough = "WT";
static char* mem_WriteProtected = "WP";
static char* mem_WriteBack = "WB";
static char* mem_Uncached = "UC-";
static char* mem_RES = "RS";


static char* fix_names[11] = {
  "IA32_MTRR_FIX64K_00000:",
  "IA32_MTRR_FIX16K_80000:",
  "IA32_MTRR_FIX16K_A0000:",
  "IA32_MTRR_FIX4K_C0000: ",
  "IA32_MTRR_FIX4K_C8000: ",
  "IA32_MTRR_FIX4K_D0000: ",
  "IA32_MTRR_FIX4K_D8000: ",
  "IA32_MTRR_FIX4K_E0000: ",
  "IA32_MTRR_FIX4K_E8000: ",
  "IA32_MTRR_FIX4K_F0000: ",
  "IA32_MTRR_FIX4K_F8000: "
};

static char* var_names[20] = {
  "IA32_MTRR_PHYSBASE0",
  "IA32_MTRR_PHYSMASK0",
  "IA32_MTRR_PHYSBASE1",
  "IA32_MTRR_PHYSMASK1",
  "IA32_MTRR_PHYSBASE2",
  "IA32_MTRR_PHYSMASK2",
  "IA32_MTRR_PHYSBASE3",
  "IA32_MTRR_PHYSMASK3",
  "IA32_MTRR_PHYSBASE4",
  "IA32_MTRR_PHYSMASK4",
  "IA32_MTRR_PHYSBASE5",
  "IA32_MTRR_PHYSMASK5",
  "IA32_MTRR_PHYSBASE6",
  "IA32_MTRR_PHYSMASK6",
  "IA32_MTRR_PHYSBASE7",
  "IA32_MTRR_PHYSMASK7",
  "IA32_MTRR_PHYSBASE8",
  "IA32_MTRR_PHYSMASK8",
  "IA32_MTRR_PHYSBASE9",
  "IA32_MTRR_PHYSMASK9"
};


static inline char* pat_type_to_str(uint64_t t)
{
  switch (t)
  {
  case 0:
    return mem_Uncacheable;

  case 1:
    return mem_WriteCombining;

  case 4:
    return mem_WriteThrough;

  case 5:
    return mem_WriteProtected;

  case 6:
    return mem_WriteBack;

  case 7:
    return mem_Uncached;

  default:
    return mem_RES;
  }
}

static inline char* mtrr_type_to_str(uint64_t t)
{
  switch (t)
  {
  case 0:
    return mem_Uncacheable;

  case 1:
    return mem_WriteCombining;

  case 4:
    return mem_WriteThrough;

  case 5:
    return mem_WriteProtected;

  case 6:
    return mem_WriteBack;

  default:
    return mem_RES;
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
  uint64_t has_gib_pages = k_cpuid_rdx(0x80000001) & BM_26;
  fprintf(stddbg, "[CPUID] 1 GiB Pages: %c\n", has_gib_pages ? 'Y' : 'N');

  // Determine how many bits to check in the variable range MTRRs.
  uint64_t mask_bits = (k_cpuid_rax(0x80000008) & 0xFF) - 12;
  uint64_t var_mask = 0;
  for (int i = 0; i < mask_bits; i++)
  {
    var_mask |= (i << (12 + i));
  }

  // Available MTRR features
  uint64_t mtrrcap = k_get_msr(IA32_MTRRCAP);
  uint64_t vcnt = mtrrcap & 0xFF;
  uint64_t has_fixed = mtrrcap & BM_8;
  uint64_t has_wc = mtrrcap & BM_10;
  uint64_t has_smrr = mtrrcap & BM_11;
  fprintf(stddbg, "[MTRR] VCNT:  %-llu\n", vcnt);
  fprintf(stddbg, "[MTRR] Fixed: %c\n", has_fixed ? 'Y' : 'N');
  fprintf(stddbg, "[MTRR] WC:    %c\n", has_wc ? 'Y' : 'N');
  fprintf(stddbg, "[MTRR] SMRR:  %c\n", has_smrr ? 'Y' : 'N');

  // Current MTRR configuration
  uint64_t mtrrdef = k_get_msr(IA32_MTRR_DEF_TYPE);
  uint64_t def_type = mtrrdef & 0xFF;
  uint64_t fixed_enabled = mtrrdef & BM_10;
  uint64_t mtrr_enabled = mtrrdef & BM_11;
  fprintf(stddbg, "[MTRR] Default Type:  %s\n", mtrr_type_to_str(def_type));
  fprintf(stddbg, "[MTRR] Fixed Enabled: %c\n", fixed_enabled ? 'Y' : 'N');
  fprintf(stddbg, "[MTRR] MTRR Enabled:  %c\n", mtrr_enabled ? 'Y' : 'N');

  if (fixed_enabled)
  {
    uint64_t fix_regs[11];

    fix_regs[0] = k_get_msr(IA32_MTRR_FIX64K_00000);
    fix_regs[1] = k_get_msr(IA32_MTRR_FIX16K_80000);
    fix_regs[2] = k_get_msr(IA32_MTRR_FIX16K_A0000);
    fix_regs[3] = k_get_msr(IA32_MTRR_FIX4K_C0000);
    fix_regs[4] = k_get_msr(IA32_MTRR_FIX4K_C8000);
    fix_regs[5] = k_get_msr(IA32_MTRR_FIX4K_D0000);
    fix_regs[6] = k_get_msr(IA32_MTRR_FIX4K_D8000);
    fix_regs[7] = k_get_msr(IA32_MTRR_FIX4K_E0000);
    fix_regs[8] = k_get_msr(IA32_MTRR_FIX4K_E8000);
    fix_regs[9] = k_get_msr(IA32_MTRR_FIX4K_F0000);
    fix_regs[10] = k_get_msr(IA32_MTRR_FIX4K_F8000);

    for (int i = 0; i < 11; i++)
    {
      fprintf(
        stddbg,
        "[MTRR] %s %s, %s, %s, %s, %s, %s, %s, %s\n",
        fix_names[i],
        mtrr_type_to_str((fix_regs[i] >> 56) & 7),
        mtrr_type_to_str((fix_regs[i] >> 48) & 7),
        mtrr_type_to_str((fix_regs[i] >> 40) & 7),
        mtrr_type_to_str((fix_regs[i] >> 32) & 7),
        mtrr_type_to_str((fix_regs[i] >> 24) & 7),
        mtrr_type_to_str((fix_regs[i] >> 16) & 7),
        mtrr_type_to_str((fix_regs[i] >> 8) & 7),
        mtrr_type_to_str(fix_regs[i] & 7)
      );
    }
  }

  if (vcnt > 0)
  {
    uint64_t var_regs[20];

    var_regs[0] = k_get_msr(IA32_MTRR_PHYSBASE0);
    var_regs[1] = k_get_msr(IA32_MTRR_PHYSMASK0);
    var_regs[2] = k_get_msr(IA32_MTRR_PHYSBASE1);
    var_regs[3] = k_get_msr(IA32_MTRR_PHYSMASK1);
    var_regs[4] = k_get_msr(IA32_MTRR_PHYSBASE2);
    var_regs[5] = k_get_msr(IA32_MTRR_PHYSMASK2);
    var_regs[6] = k_get_msr(IA32_MTRR_PHYSBASE3);
    var_regs[7] = k_get_msr(IA32_MTRR_PHYSMASK3);
    var_regs[8] = k_get_msr(IA32_MTRR_PHYSBASE4);
    var_regs[9] = k_get_msr(IA32_MTRR_PHYSMASK4);
    var_regs[10] = k_get_msr(IA32_MTRR_PHYSBASE5);
    var_regs[11] = k_get_msr(IA32_MTRR_PHYSMASK5);
    var_regs[12] = k_get_msr(IA32_MTRR_PHYSBASE6);
    var_regs[13] = k_get_msr(IA32_MTRR_PHYSMASK6);
    var_regs[14] = k_get_msr(IA32_MTRR_PHYSBASE7);
    var_regs[15] = k_get_msr(IA32_MTRR_PHYSMASK7);
    var_regs[16] = k_get_msr(IA32_MTRR_PHYSBASE8);
    var_regs[17] = k_get_msr(IA32_MTRR_PHYSMASK8);
    var_regs[18] = k_get_msr(IA32_MTRR_PHYSBASE9);
    var_regs[19] = k_get_msr(IA32_MTRR_PHYSMASK9);



    for (int i = 0; i < vcnt; i++)
    {
      if (var_regs[i * 2 + 1] & BM_11)
      {
        uint64_t physbase = var_regs[i * 2] & var_mask;
        uint64_t physmask = var_regs[i * 2 + 1] & var_mask;

        fprintf(stddbg, "[MTRR] n: %d, Start: %.16llX, ", i, physbase);

        uint64_t addr_test = physbase;
        while ((addr_test & physmask) == physbase)
        {
          addr_test += 4096;
        }
        fprintf(stddbg, "End: %.16llX, ", addr_test - 1);
        fprintf(stddbg, "Type: %s\n", mtrr_type_to_str(var_regs[i * 2] & 0xFF));
      }
    }
  }

  // Disable MTRRs if they're enabled
  // NOTE: This is a workaround for having to verify the memory types
  // of various regions of physical address space when mapping addresses.
  // if (mtrr_enabled)
  // {
  //   k_set_msr(IA32_MTRR_DEF_TYPE, mtrrdef & ~BM_11);

  //   // Verify that we successfully updated the IA32_MTRR_DEF_TYPE register.
  //   uint64_t mtrr_still_enabled = k_get_msr(IA32_MTRR_DEF_TYPE);
  //   if (mtrr_still_enabled & BM_11)
  //   {
  //     fprintf(stddbg, "[MTRR] failed to disable MTRRs\n");
  //     for (;;);
  //   }
  //   else
  //   {
  //     fprintf(stddbg, "[MTRR] successfully disabled MTRRs\n");
  //   }
  // }

  // Read the PAT
  uint64_t pat = k_get_msr(IA32_PAT);
  fprintf(stddbg, "[PAT] PA0: %s\n", pat_type_to_str(pat & 7));
  fprintf(stddbg, "[PAT] PA1: %s\n", pat_type_to_str((pat >> 8) & 7));
  fprintf(stddbg, "[PAT] PA2: %s\n", pat_type_to_str((pat >> 16) & 7));
  fprintf(stddbg, "[PAT] PA3: %s\n", pat_type_to_str((pat >> 24) & 7));
  fprintf(stddbg, "[PAT] PA4: %s\n", pat_type_to_str((pat >> 32) & 7));
  fprintf(stddbg, "[PAT] PA5: %s\n", pat_type_to_str((pat >> 40) & 7));
  fprintf(stddbg, "[PAT] PA6: %s\n", pat_type_to_str((pat >> 48) & 7));
  fprintf(stddbg, "[PAT] PA7: %s\n", pat_type_to_str((pat >> 56) & 7));


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
