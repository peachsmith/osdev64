#include "osdev64/firmware.h"
#include "osdev64/bitmask.h"
#include "osdev64/memory.h"
#include "osdev64/paging.h"

#include "klibc/stdio.h"

// The MADT
k_byte* g_madt = NULL;

// The MCFG
k_byte* g_mcfg = NULL;

extern k_byte* g_sys_rsdp;
extern int g_sys_acpi_ver;


// ISO polarity and trigger types
static char* iso_bus = "bus";
static char* iso_res = "res";
static char* iso_high = "high";
static char* iso_low = "low";
static char* iso_edge = "edge";
static char* iso_level = "level";
static char* iso_err = "err";

/**
 * Converts an ISO polarity flag into a string.
 * The flags are assumed to be shifted all the way to the right.
 * So if the flags come from bits [3:2], then they should be
 * shifted to the right by 2.
 *
 * Params:
 *   uint64_t - the flags from the MADT entry
 *
 * Returns:
 *   char* - a pointer to a NUL-terminated string
 */
static inline char* iso_polarity_str(uint64_t f)
{
  switch (f)
  {
  case 0:
    return iso_bus;

  case 1:
    return iso_high;

  case 2:
    return iso_res;

  case 3:
    return iso_low;

  default: // invalid flag value
    return iso_err;
  }
}

/**
 * Converts an ISO trigger mode flag into a string.
 * The flags are assumed to be shifted all the way to the right.
 * So if the flags come from bits [3:2], then they should be
 * shifted to the right by 2.
 *
 * Params:
 *   uint64_t - the flags from the MADT entry
 *
 * Returns:
 *   char* - a pointer to a NUL-terminated string
 */
static inline char* iso_trigger_str(uint64_t f)
{
  switch (f)
  {
  case 0:
    return iso_bus;

  case 1:
    return iso_edge;

  case 2:
    return iso_res;

  case 3:
    return iso_level;

  default: // invalid flag value
    return iso_err;
  }
}




void k_acpi_init()
{
  k_byte* rsdt;
  k_byte* madt = NULL;

  // Get the RSDP from UEFI firmware.
  if (!g_sys_acpi_ver)
  {
    fprintf(stddbg, "[ERROR] failed to locate RSDP\n");
    HANG();
  }

  // Print the RSDP signature (bytes 0 - 7)
  fprintf(stddbg, "RSDP Signature: %c%c%c%c%c%c%c%c\n",
    g_sys_rsdp[0],
    g_sys_rsdp[1],
    g_sys_rsdp[2],
    g_sys_rsdp[3],
    g_sys_rsdp[4],
    g_sys_rsdp[5],
    g_sys_rsdp[6],
    g_sys_rsdp[7]
  );

  // Print the OEM ID (bytes 9 - 14)
  fprintf(stddbg, "OEMID: %c%c%c%c%c%c\n",
    g_sys_rsdp[9],
    g_sys_rsdp[10],
    g_sys_rsdp[11],
    g_sys_rsdp[12],
    g_sys_rsdp[13],
    g_sys_rsdp[14]
  );

  // If we're using ACPI >= 2.0, get the XSDT address,
  // otherwise get the RSDT address.
  if (g_sys_acpi_ver == 2)
  {
    // Alright, this probably looks a little silly.
    // Byte 24 of the RSDP structure is the beginning of 8
    // bytes of memory which contain the address of the XSDT.
    // So first, we cast the address of byte 24 of the RSDP as a
    // pointer to a 64-bit unsigned integer.
    // We then dereference that pointer to get the value, and we
    // cast that value as a pointer to the XSDT.
    rsdt = (k_byte*)*(uint64_t*)(g_sys_rsdp + 24);
  }
  else
  {
    // If we're using the 32-bit RSDP, cast it as a 64-bit
    // integer before casting as a pointer, since addresses
    // are 64 bits.
    rsdt = (k_byte*)(uint64_t) * (uint32_t*)(g_sys_rsdp + 16);
  }

  // All SDTs start with an SDT header.
  // The SDT header is 36 bytes in the following structure:
  // ---------------------------------------------------------------------
  // bytes       purpose
  // ---------------------------------------------------------------------
  // 0 -3        signatrure
  // 4 - 7       length of the table (including the header)
  // 8           revision number
  // 9           checksum
  // 10 -15      OEM ID
  // 16 - 23     OEM table ID
  // 24 - 27     OEM revision number
  // 28 - 31     creator ID
  // 32 - 35     creator revision
  // ---------------------------------------------------------------------

  // Print the RSDT signature (bytes 0 - 4)
  fprintf(stddbg, "RSDT signature %c%c%c%c\n",
    rsdt[0],
    rsdt[1],
    rsdt[2],
    rsdt[3]
  );

  // Print the OEM ID (bytes 10 - 15)
  fprintf(stddbg, "RSDT OEMID: %c%c%c%c%c%c\n",
    rsdt[10],
    rsdt[11],
    rsdt[12],
    rsdt[13],
    rsdt[14],
    rsdt[15]
  );

  // Print the OEM table ID (bytes 16 - 23)
  fprintf(stddbg, "RSDT OEM Table ID: %c%c%c%c%c%c%c%c\n",
    rsdt[16],
    rsdt[17],
    rsdt[18],
    rsdt[19],
    rsdt[20],
    rsdt[21],
    rsdt[22],
    rsdt[23]
  );

  // Get the length of the RSDT.
  uint32_t rsdt_len = *(uint32_t*)(rsdt + 4);

  k_byte rsdt_check = 0;
  for (uint32_t i = 0; i < rsdt_len; i++)
  {
    rsdt_check += rsdt[i];
  }

  fprintf(stddbg, "RSDT checksum: %u\n", rsdt_check);

  // Print the signatures of all the tables in the RSDT.
  for (uint32_t i = 0; i < (rsdt_len - 36) / 8; i++)
  {
    // Use our byte-to-pointer trick to get the pointer to
    // the next SDT.
    k_byte* sdt = (k_byte*)(*(uint64_t*)(rsdt + 36 + i * 8));

    fprintf(stddbg, "SDT signature %c%c%c%c\n",
      sdt[0],
      sdt[1],
      sdt[2],
      sdt[3]
    );

    // The signature "APIC" indicates the MADT.
    if (sdt[0] == 'A' && sdt[1] == 'P' && sdt[2] == 'I' && sdt[3] == 'C')
    {
      uint32_t len = *(uint32_t*)(sdt + 4);

      // Calculate the checksum.
      k_byte check = 0;
      for (uint32_t j = 0; j < len; j++)
      {
        check += sdt[j];
      }

      // If we validated the checksum, then we found the MADT.
      if (check == 0)
      {
        fprintf(stddbg, "[ACPI] located the MADT\n");

        // Allocate memory to store the MADT
        g_madt = k_memory_alloc_pages(len / 0x1000 + 1);
        if (g_madt == NULL)
        {
          fprintf(stddbg, "[ERROR] failed to allocate memory for the MADT\n");
          HANG();
        }

        // Copy the MADT into our dynamic memory.
        for (uint32_t j = 0; j < len; j++)
        {
          g_madt[j] = sdt[j];
        }
      }
    }

    // The signature "MCFG" indicates the MADT.
    if (sdt[0] == 'M' && sdt[1] == 'C' && sdt[2] == 'F' && sdt[3] == 'G')
    {
      uint32_t len = *(uint32_t*)(sdt + 4);

      // Calculate the checksum.
      k_byte check = 0;
      for (uint32_t j = 0; j < len; j++)
      {
        check += sdt[j];
      }

      // If we validated the checksum, then we found the MADT.
      if (check == 0)
      {
        fprintf(stddbg, "[ACPI] located the MCFG\n");

        // Allocate memory to store the MADT
        g_mcfg = k_memory_alloc_pages(len / 0x1000 + 1);
        if (g_mcfg == NULL)
        {
          fprintf(stddbg, "[ERROR] failed to allocate memory for the MCFG\n");
          HANG();
        }

        // Copy the MCFG into our dynamic memory.
        for (uint32_t j = 0; j < len; j++)
        {
          g_mcfg[j] = sdt[j];
        }
      }
    }
  }
}


void k_acpi_print_madt()
{
  if (g_madt == NULL)
  {
    fprintf(stddbg, "[ACPI] No MADT detected\n");
    return;
  }

  uint64_t lapic_base = 0;
  uint64_t ioapic_base = 0;

  uint32_t table_len = *((uint32_t*)(g_madt + 4));
  uint32_t lapic32 = *((uint32_t*)(g_madt + 36));
  uint32_t flags = *((uint32_t*)(g_madt + 40));

  // Set the local APIC base to be the value we found before the
  // MADT entries list. We may replace this with a 
  lapic_base = (uint64_t)lapic32;

  // Calculate the checksum.
  k_byte madt_check = 0;
  for (uint32_t i = 0; i < table_len; i++)
  {
    madt_check += g_madt[i];
  }

  fprintf(stddbg, "[ACPI] MADT Checksum: %u\n", madt_check);
  fprintf(stddbg, "[ACPI] MADT Length: %d\n", table_len);
  fprintf(stddbg, "[ACPI] MADT 32-bit Local APIC Base: %X\n", lapic_base);
  fprintf(stddbg, "[ACPI] MADT Flags: %X\n", flags);

  // Print the entries of the MADT.
  // The index i represents the byte offset from the start of the
  // entry list.
  // The entry list starts at offset 44.
  for (uint32_t i = 0; i < table_len - 44;)
  {
    k_byte* entry = &g_madt[44 + i];

    uint8_t entry_type = *((uint8_t*)(entry));
    uint8_t entry_len = *((uint8_t*)(entry + 1));

    switch (entry_type)
    {
    case 0:
    {
      uint8_t lapic_ver = *((uint8_t*)(entry + 3));
      fprintf(stddbg, "[ACPI] Local APIC: { ID: %u }\n", lapic_ver);
    }
    break;

    case 1:
    {
      uint8_t id = *((uint8_t*)(entry + 2));
      uint64_t ioapic32 = (uint64_t)(*((uint32_t*)(entry + 4)));
      uint32_t gsi = *((uint32_t*)(entry + 8));

      if (gsi == 0)
      {
        fprintf(stddbg, "[ACPI] I/O APIC: { ID: %u, Base: %X, GSI: %u }\n",
          id,
          (uint64_t)ioapic32,
          gsi
        );
      }
    }
    break;

    case 2:
    {
      uint8_t bus_src = *((uint8_t*)(entry + 2));
      uint8_t irq_src = *((uint8_t*)(entry + 3));
      uint32_t gsi = *((uint32_t*)(entry + 4));
      uint16_t flags = *((uint16_t*)(entry + 8));

      // polarity and trigger mode
      uint64_t pol = ((uint64_t)flags & BM_2_BITS);
      uint64_t trig = ((uint64_t)flags & (BM_2_BITS << 2)) >> 2;

      fprintf(stddbg, "[ACPI] ISO: { Bus: %3u, IRQ: %3u, GSI: %3u Pol: %5s, Trig: %5s }\n",
        bus_src,
        irq_src,
        gsi,
        iso_polarity_str(pol),
        iso_trigger_str(trig)
      );
    }
    break;

    case 4:
    {
      uint8_t id = *((uint8_t*)(entry + 2));
      uint8_t flags = *((uint8_t*)(entry + 3));
      uint8_t lint = *((uint8_t*)(entry + 4));

      // polarity and trigger mode
      uint64_t pol = ((uint64_t)flags & BM_2_BITS);
      uint64_t trig = ((uint64_t)flags & (BM_2_BITS << 2)) >> 2;
      fprintf(stddbg, "[ACPI] NMI: { ID: %3X, Pol: %5s, Trig: %5s, LINT: %u }\n",
        id,
        iso_polarity_str(pol),
        iso_trigger_str(trig),
        lint
      );
    }
    break;

    case 5:
    {
      uint64_t lapic_ovr = *((uint64_t*)(entry + 4));
      fprintf(stddbg, "[ACPI] Local APIC Override: { Addr: %p }\n", lapic_ovr);
    }
    break;

    default:
      fprintf(stddbg, "[ACPI] Unknown: { type: %u } ", entry_type);
      break;
    }

    i += entry_len;
  }
}
