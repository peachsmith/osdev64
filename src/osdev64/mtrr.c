#include "osdev64/mtrr.h"
#include "osdev64/msr.h"
#include "osdev64/bitmask.h"
#include "osdev64/cpuid.h"

#include "klibc/stdio.h"

// memory caching types
static char* mem_Uncacheable = "UC";
static char* mem_WriteCombining = "WC";
static char* mem_WriteThrough = "WT";
static char* mem_WriteProtected = "WP";
static char* mem_WriteBack = "WB";
static char* mem_Uncached = "UC-";
static char* mem_RES = "RS";

// fixed range MTRR names
static char* mtrr_fix_names[11] = {
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

// variable range MTRR names
static char* mtrr_var_names[20] = {
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


void k_mtrr_print_all()
{
  // Determine how many bits to check in the variable range MTRRs.
  uint64_t mask_bits = (k_cpuid_rax(0x80000008) & 0xFF) - 12;
  uint64_t var_mask = 0;
  for (int i = 0; i < mask_bits; i++)
  {
    var_mask |= (i << (12 + i));
  }

  // Available MTRR features
  uint64_t mtrrcap = k_msr_get(IA32_MTRRCAP);
  uint64_t vcnt = mtrrcap & 0xFF;
  uint64_t has_fixed = mtrrcap & BM_8;
  uint64_t has_wc = mtrrcap & BM_10;
  uint64_t has_smrr = mtrrcap & BM_11;
  fprintf(stddbg, "[MTRR] VCNT:  %-llu\n", vcnt);
  fprintf(stddbg, "[MTRR] Fixed: %c\n", has_fixed ? 'Y' : 'N');
  fprintf(stddbg, "[MTRR] WC:    %c\n", has_wc ? 'Y' : 'N');
  fprintf(stddbg, "[MTRR] SMRR:  %c\n", has_smrr ? 'Y' : 'N');

  // Current MTRR configuration
  uint64_t mtrrdef = k_msr_get(IA32_MTRR_DEF_TYPE);
  uint64_t def_type = mtrrdef & 0xFF;
  uint64_t fixed_enabled = mtrrdef & BM_10;
  uint64_t mtrr_enabled = mtrrdef & BM_11;
  fprintf(stddbg, "[MTRR] Default Type:  %s\n", mtrr_type_to_str(def_type));
  fprintf(stddbg, "[MTRR] Fixed Enabled: %c\n", fixed_enabled ? 'Y' : 'N');
  fprintf(stddbg, "[MTRR] MTRR Enabled:  %c\n", mtrr_enabled ? 'Y' : 'N');

  if (fixed_enabled)
  {
    uint64_t fix_regs[11];

    fix_regs[0] = k_msr_get(IA32_MTRR_FIX64K_00000);
    fix_regs[1] = k_msr_get(IA32_MTRR_FIX16K_80000);
    fix_regs[2] = k_msr_get(IA32_MTRR_FIX16K_A0000);
    fix_regs[3] = k_msr_get(IA32_MTRR_FIX4K_C0000);
    fix_regs[4] = k_msr_get(IA32_MTRR_FIX4K_C8000);
    fix_regs[5] = k_msr_get(IA32_MTRR_FIX4K_D0000);
    fix_regs[6] = k_msr_get(IA32_MTRR_FIX4K_D8000);
    fix_regs[7] = k_msr_get(IA32_MTRR_FIX4K_E0000);
    fix_regs[8] = k_msr_get(IA32_MTRR_FIX4K_E8000);
    fix_regs[9] = k_msr_get(IA32_MTRR_FIX4K_F0000);
    fix_regs[10] = k_msr_get(IA32_MTRR_FIX4K_F8000);

    for (int i = 0; i < 11; i++)
    {
      fprintf(
        stddbg,
        "[MTRR] %s %s, %s, %s, %s, %s, %s, %s, %s\n",
        mtrr_fix_names[i],
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

    var_regs[0] = k_msr_get(IA32_MTRR_PHYSBASE0);
    var_regs[1] = k_msr_get(IA32_MTRR_PHYSMASK0);
    var_regs[2] = k_msr_get(IA32_MTRR_PHYSBASE1);
    var_regs[3] = k_msr_get(IA32_MTRR_PHYSMASK1);
    var_regs[4] = k_msr_get(IA32_MTRR_PHYSBASE2);
    var_regs[5] = k_msr_get(IA32_MTRR_PHYSMASK2);
    var_regs[6] = k_msr_get(IA32_MTRR_PHYSBASE3);
    var_regs[7] = k_msr_get(IA32_MTRR_PHYSMASK3);
    var_regs[8] = k_msr_get(IA32_MTRR_PHYSBASE4);
    var_regs[9] = k_msr_get(IA32_MTRR_PHYSMASK4);
    var_regs[10] = k_msr_get(IA32_MTRR_PHYSBASE5);
    var_regs[11] = k_msr_get(IA32_MTRR_PHYSMASK5);
    var_regs[12] = k_msr_get(IA32_MTRR_PHYSBASE6);
    var_regs[13] = k_msr_get(IA32_MTRR_PHYSMASK6);
    var_regs[14] = k_msr_get(IA32_MTRR_PHYSBASE7);
    var_regs[15] = k_msr_get(IA32_MTRR_PHYSMASK7);
    var_regs[16] = k_msr_get(IA32_MTRR_PHYSBASE8);
    var_regs[17] = k_msr_get(IA32_MTRR_PHYSMASK8);
    var_regs[18] = k_msr_get(IA32_MTRR_PHYSBASE9);
    var_regs[19] = k_msr_get(IA32_MTRR_PHYSMASK9);



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

  // Read the PAT
  uint64_t pat = k_msr_get(IA32_PAT);
  fprintf(stddbg, "[PAT] PA0: %s\n", pat_type_to_str(pat & 7));
  fprintf(stddbg, "[PAT] PA1: %s\n", pat_type_to_str((pat >> 8) & 7));
  fprintf(stddbg, "[PAT] PA2: %s\n", pat_type_to_str((pat >> 16) & 7));
  fprintf(stddbg, "[PAT] PA3: %s\n", pat_type_to_str((pat >> 24) & 7));
  fprintf(stddbg, "[PAT] PA4: %s\n", pat_type_to_str((pat >> 32) & 7));
  fprintf(stddbg, "[PAT] PA5: %s\n", pat_type_to_str((pat >> 40) & 7));
  fprintf(stddbg, "[PAT] PA6: %s\n", pat_type_to_str((pat >> 48) & 7));
  fprintf(stddbg, "[PAT] PA7: %s\n", pat_type_to_str((pat >> 56) & 7));
}