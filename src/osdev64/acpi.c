#include "osdev64/firmware.h"
#include "osdev64/bitmask.h"
#include "osdev64/memory.h"
#include "osdev64/paging.h"

#include "klibc/stdio.h"

// The MADT
k_byte* g_madt = NULL;

// The MCFG
k_byte* g_mcfg = NULL;


typedef struct mcfg_entry {
  uint64_t phys_base;
  uint64_t virt_base;
  uint16_t group;
  uint8_t bus_start;
  uint8_t bus_end;
}mcfg_entry;

static mcfg_entry pci_base;

// k_regn g_pci_phys = 0;
// k_regn g_pci_virt = 0;

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

static void print_pci_device(uint64_t phys, uint64_t bd, uint64_t bus)
{
  uint64_t virt = k_paging_map_range(phys + bd, phys + bd + 0x1000);

  if (virt == 0)
  {
    fprintf(stddbg, "[ERROR] failed to map PCI device\n");
  }
  else
  {
    k_byte* dev = (k_byte*)virt;
    uint16_t deviceID = (((*(uint32_t*)(dev)) & 0xFFFF0000) >> 16);
    uint16_t vendorID = ((*(uint32_t*)(dev)) & 0xFFFF);
    uint8_t clas = (((*(uint32_t*)(dev + 8)) & 0xFF000000) >> 24);
    uint8_t subclass = (((*(uint32_t*)(dev + 8)) & 0xFF0000) >> 16);
    uint8_t header_type = (((*(uint32_t*)(dev + 12)) & 0xFF0000) >> 16);

    // Header Type:
    // 0 general device
    // 1 PCI to PCI bridge
    // 2 card bus bridge
    // if bit 7 is set, then the device has multiple functions

    if (vendorID != 0xFFFF)
    {
      fprintf(stddbg,
        "[PCI DEV] Ven: %4X, Dev: %4X, Head: %2X, Class: %2X, Sub: %2X\n",
        vendorID,
        deviceID,
        header_type & 0x7F,
        clas,
        subclass
      );

      if (header_type & 0x80)
      {
        fprintf(stddbg, "--------------------------------------------------\n");
        // There are 8 functions for each device.
        for (uint64_t f = 0; f < 8; f++)
        {
          uint64_t phys_fun = phys + (bd | (f << 12));
          uint64_t virt_fun = k_paging_map_range(phys_fun, phys_fun + 0x1000);

          if (virt_fun == 0)
          {
            fprintf(stddbg, "[ERROR] failed to map PCI function\n");
          }
          else
          {
            uint32_t* fun = (uint32_t*)virt_fun;
            uint16_t fdeviceID = (((*(uint32_t*)(fun)) & 0xFFFF0000) >> 16);
            uint16_t fvendorID = ((*(uint32_t*)(fun)) & 0xFFFF);
            uint8_t fclas = (((*(uint32_t*)(fun + 8)) & 0xFF000000) >> 24);
            uint8_t fsubclass = (((*(uint32_t*)(fun + 8)) & 0xFF0000) >> 16);
            uint8_t fheader_type = (((*(uint32_t*)(fun + 12)) & 0xFF0000) >> 16);

            if (fvendorID != 0xFFFF && fdeviceID != 0xFFFF)
            {
              fprintf(stddbg,
                "[PCI FUN] Ven: %4X, Dev: %4X, Head: %2X, Class: %2X, Sub: %2X\n",
                fvendorID,
                fdeviceID,
                fheader_type & 0x7F,
                fclas,
                fsubclass
              );
            }

            k_paging_unmap_range(virt_fun);
          }
        }
        fprintf(stddbg, "--------------------------------------------------\n");
      }
    }

    k_paging_unmap_range(virt);
  }
}

static void print_pci()
{
  if (pci_base.virt_base == 0)
  {
    fprintf(stddbg, "no PCI configuration space found\n");
    return;
  }

  fprintf(
    stddbg,
    "[PCI] bus start: %u, end: %u\n",
    pci_base.bus_start,
    pci_base.bus_end
  );

  uint64_t pci_count = 0;

  // Iterate over all busses in the configuration space.
  for (uint64_t b = pci_base.bus_start; b < pci_base.bus_end; b++)
  {
    // pci_base.phys_base;
    uint64_t phys = 0;
    // phys = (b << 20);

    // There are 32 devices per bus.
    for (uint64_t d = 0; d < 32; d++)
    {
      // phys += (d << 15);

      print_pci_device(pci_base.phys_base, ((b << 20) | (d << 15)), b);

      // There are 8 functions for each device.
      // for (uint64_t f = 0; f < 8; f++)
      // {
      //   pci_count++; // tmp

      //   phys += (f << 12);
      // }
    }
  }

  fprintf(stddbg, "[PCI] %llu PCI functions scanned\n", pci_count);
}


void k_acpi_read_mcfg()
{
  if (g_mcfg == NULL)
  {
    fprintf(stddbg, "[ACPI] No MCFG detected\n");
    return;
  }

  uint32_t table_len = *((uint32_t*)(g_mcfg + 4));

  // Calculate the checksum.
  k_byte mcfg_check = 0;
  for (uint32_t i = 0; i < table_len; i++)
  {
    mcfg_check += g_mcfg[i];
  }

  fprintf(stddbg, "[ACPI] MCFG Checksum: %u\n", mcfg_check);
  fprintf(stddbg, "[ACPI] MCFG Length: %d\n", table_len);

  // Print the entries of the MCFG.
  // The index i represents the byte offset from the start of the
  // entry list.
  // The entry list starts at offset 44.
  // Each entry is 16 bytes.
  for (uint32_t i = 0; i < table_len - 44;)
  {
    k_byte* entry = &g_mcfg[44 + i];

    uint64_t base = *((uint64_t*)(entry));
    uint16_t seg_group = *((uint16_t*)(entry + 8));
    uint8_t bus_start = *((uint8_t*)(entry + 10));
    uint8_t bus_end = *((uint8_t*)(entry + 11));

    fprintf(
      stddbg,
      "[ACPI] MCFG base: %llX, group: %u, start bus: %d, end bus: %d\n",
      base,
      seg_group,
      bus_start,
      bus_end
    );

    if (pci_base.phys_base == 0)
    {
      pci_base.phys_base = base;
      pci_base.bus_start = bus_start;
      pci_base.bus_end = bus_end;
    }

    // Increment i by the size of an MCFG entry.
    i += 16;
  }

  if (pci_base.phys_base)
  {
    pci_base.virt_base = k_paging_map_range(pci_base.phys_base, pci_base.phys_base + 0x1000);

    if (pci_base.virt_base == 0)
    {
      fprintf(
        stddbg,
        "[ERROR] could not map PCI configuration space into virtual memory\n"
      );
    }
    else
    {
      fprintf(
        stddbg,
        "[ACPI] PCI phys base: %llX, virt base: %llX\n",
        pci_base.phys_base,
        pci_base.virt_base
      );

      print_pci();
    }
  }
  else
  {
    fprintf(
      stddbg,
      "[ERROR] could not find PCI configuration space\n"
    );
  }
}
