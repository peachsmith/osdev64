#include "osdev64/pci.h"
#include "osdev64/memory.h"
#include "osdev64/paging.h"

#include "klibc/stdio.h"


extern k_byte* g_mcfg;

typedef struct mcfg_entry {
  uint64_t phys_base;
  uint64_t virt_base;
  uint16_t group;
  uint8_t bus_start;
  uint8_t bus_end;
}mcfg_entry;

static mcfg_entry pci_base;


static void pci_print_bdf();



// bus/device/function (B/D/F or BDF) address structure
//
//   [63:28]   [27:20]  [19:15]    [14:12]     [11:0]
// +--------+---------+--------+----------+----------+
// |  base  |   bus   | device | function |  offset  |
// +--------+---------+--------+----------+----------+
//  40 bits   8 bits    5 bits   3 bits     12 bits
//
// Given a base physical address (p), a bus (b), a device (d), and
// a function (f), the physical base address of the 4KiB configuration
// space for the given b/d/f can be calculated by
// c = p + ((b << 20) | (d << 15) | (f << 12))

static int print_bdf(uint8_t b, uint8_t d, uint8_t f)
{
  uint64_t p = pci_base.phys_base;

  p += (
    ((uint64_t)b << 20)
    | ((uint64_t)d << 15)
    | ((uint64_t)f << 12)
    );

  // Map the 4KiB configuration space into virtual memory.
  k_regn v = k_paging_map_range(p, p + 0x1000);
  if (v == 0)
  {
    fprintf(stddbg, "[ERROR] failed to map PCI b/d/f into virtual memory\n");
    return 0;
  }

  uint8_t* cfg = (uint8_t*)v;

  uint16_t ven = *(uint16_t*)cfg;       // vendor ID
  uint16_t dev = *(uint16_t*)(cfg + 2); // device ID
  uint8_t sub = *(uint8_t*)(cfg + 10);  // subclass
  uint8_t cls = *(uint8_t*)(cfg + 11);  // class
  uint8_t hea = *(uint8_t*)(cfg + 14);  // header type

  // Unmap the configuration space, since we're not using it yet.
  k_paging_unmap_range(v);

  if (ven != 0xFFFF && dev != 0xFFFF)
  {
    fprintf(stddbg,
      "| (%2X/%2X/%2X) %-4X  %-4X  %-5X  %-8X  %-6X |\n",
      b, d, f, ven, dev, cls, sub, hea & 0x7F
    );
  }
  else
  {
    return 0;
  }

  return (hea & 0x80) ? 1 : 2;
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

  // table start
  fprintf(stddbg, "+------------------------------------------------+\n");
  fprintf(stddbg, "| (b/d/f)    VID   DID   Class  Subclass  Header |\n");
  fprintf(stddbg, "+------------------------------------------------+\n");

  // Iterate over all busses in the configuration space.
  for (uint8_t b = pci_base.bus_start; b < pci_base.bus_end; b++)
  {
    // There are 32 devices per bus.
    for (uint8_t d = 0; d < 32; d++)
    {
      // All PCI devices are required to implement function 0.
      int res = print_bdf(b, d, 0);
      if (res == 1)
      {
        // There are 8 functions for each device.
        for (uint8_t f = 1; f < 8; f++)
        {
          print_bdf(b, d, f);
        }
      }

      if (res && b < pci_base.bus_end - 1)
      {
        // device separator
        fprintf(stddbg, "|------------------------------------------------|\n");
      }
    }
  }

  // table end
  fprintf(stddbg, "+------------------------------------------------+\n");
}

void k_pci_init()
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
