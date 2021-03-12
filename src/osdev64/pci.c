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

typedef struct pci_id_map {
  uint16_t vendorID;
  uint16_t deviceID;
  char name[48];
}pci_id_map;


static mcfg_entry pci_base;



/**
 * Gathers the available devices into a list.
 */
static void pci_collect();

/**
 * Checks a PCI endpoint for a valid device and stores it in a list.
 * If the device is a multi function device, this function returns 1,
 * otherwise this function returns 0.
 *
 * Params:
 *   uint8_t - bus index
 *   uint8_t - device index
 *   uint8_t - function index
 *
 * Returns:
 *   int - 1 if the device is multi function, otherwise 0
 */
static int pci_check_bdf(uint8_t, uint8_t, uint8_t);

/**
 * Prints information about a PCI device.
 *
 * Params:
 *   pci_dev* - a pointer to a PCI device
 */
static void pci_describe(pci_dev* dev);


// a mass storage device
pci_dev* k_storage_dev = NULL;

// very primitive list of devices
static pci_dev* devices[10];
static int dev_count = 0;


// PCI device descriptions
// Ven  Dev  Vendor Description  Device Description
// 8086 7190 Intel Corporation   440BX/ZX/DX - 82443BX/ZX/DX Host bridge
// 8086 7191 Intel Corporation   440BX/ZX/DX - 82443BX/ZX/DX AGP bridge
// 8086 7110 Intel Corporation   82371AB/EB/MB PIIX4 ISA
// 8086 7111 Intel Corporation   82371AB/EB/MB PIIX4 IDE
// 8086 7113 Intel Corporation   82371AB/EB/MB PIIX4 ACPI
// 15AD 740  VMWare              Virtual Machine Communication Interface
// 15AD 405  VMWare              SVGA II Adapter
// 15AD 790  VMWare              PCI bridge
// 15AD 7A0  VMWare              PCI Express Root Port
// 8086 100F Intel Corporation   82545EM Gigabit Ethernet Controller (Copper)
// 1274 1371 Ensoniq             ES1371/ES1373 / Creative Labs CT2518
// 15AD 774  VMWare              USB1.1 UHCI Controller
// 15AD 770  VMWare              USB2 EHCI Controller



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



static int pci_check_bdf(uint8_t b, uint8_t d, uint8_t f)
{
  pci_dev* dev = NULL;

  uint64_t bdf = (
    ((uint64_t)b << 20)
    | ((uint64_t)d << 15)
    | ((uint64_t)f << 12)
    );

  uint64_t endpoint = pci_base.virt_base + bdf;

  dev = (pci_dev*)endpoint;

  uint16_t venID = *(uint16_t*)dev;       // vendor ID
  uint16_t devID = *(uint16_t*)(dev + 2); // device ID
  // uint8_t sub = *(uint8_t*)(dev + 10);  // subclass
  uint8_t cls = *(uint8_t*)(dev + 11);  // class
  uint8_t hea = *(uint8_t*)(dev + 14);  // header type

  // Skip invalid devices.
  if (venID == 0xFFFF || devID == 0xFFFF)
  {
    return 0;
  }

  // Only add devices with a class of 1, 2, 3, 4, 12, or 13
  // to the list.
  if (cls == 0 || (cls > 4 && cls < 0xC) || cls > 0xD)
  {
    return hea & 0x80;
  }

  // Add the device to the list.
  if (dev_count < 10)
  {
    devices[dev_count++] = dev;
  }

  // Locate a mass storage device.
  if (cls == PCI_CLASS_STOR && k_storage_dev == NULL)
  {
    k_storage_dev = dev;
  }

  return hea & 0x80;
}


static void pci_describe(pci_dev* dev)
{
  uint16_t venID = *(uint16_t*)dev;       // vendor ID
  uint16_t devID = *(uint16_t*)(dev + 2); // device ID
  uint8_t sub = *(uint8_t*)(dev + 10);  // subclass
  uint8_t dclass = *(uint8_t*)(dev + 11);  // class
  uint8_t dheader = *(uint8_t*)(dev + 14);  // header type

  fprintf(stddbg,
    "VID: %4X, DID: %4X, Class: %2X Subclass: %2X, Header: %2X\n",
    venID, devID, dclass, sub, dheader & 0x7F
  );
}

static void pci_collect()
{
  if (pci_base.virt_base == 0)
  {
    fprintf(stddbg, "no PCI configuration space found\n");
    return;
  }

  // Iterate over all busses in the configuration space.
  for (uint8_t b = pci_base.bus_start; b < pci_base.bus_end; b++)
  {
    // There are 32 devices per bus.
    for (uint8_t d = 0; d < 32; d++)
    {
      // All PCI devices are required to implement function 0.
      if (pci_check_bdf(b, d, 0))
      {
        // There are 8 functions for each device.
        for (uint8_t f = 1; f < 8; f++)
        {
          pci_check_bdf(b, d, f);
        }
      }
    }
  }
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

  if (!pci_base.phys_base)
  {
    fprintf(
      stddbg,
      "[ERROR] could not find PCI configuration space\n"
    );
    return;
  }

  uint64_t cfg_max = (
    (((uint64_t)(pci_base.bus_end - 1)) << 20)
    | ((uint64_t)31 << 15)
    | ((uint64_t)7 << 12)
    );

  pci_base.virt_base = k_paging_map_range(pci_base.phys_base, pci_base.phys_base + cfg_max);

  if (pci_base.virt_base == 0)
  {
    fprintf(
      stddbg,
      "[ERROR] could not map PCI configuration space into virtual memory\n"
    );
    return;
  }

  fprintf(
    stddbg,
    "[ACPI] PCI phys base: %llX, virt base: %llX\n",
    pci_base.phys_base,
    pci_base.virt_base
  );

  k_paging_print_ledger();

  // Gather all available PCI devices into a list.
  pci_collect();

  fprintf(stddbg, "Device List\n");

  // Print out the device classes.
  for (int j = 0; j < dev_count; j++)
  {
    uint16_t venID = *(uint16_t*)devices[j];
    uint16_t devID = *(uint16_t*)(devices[j] + 2);
    uint8_t cls = *(uint8_t*)(devices[j] + 11);

    switch (cls)
    {
    case PCI_CLASS_STOR:
      fprintf(stddbg, "  Class: storage     VID/DID: %4X / %4X\n", venID, devID);
      break;

    case PCI_CLASS_NETW:
      fprintf(stddbg, "  Class: network     VID/DID: %4X / %4X\n", venID, devID);
      break;

    case PCI_CLASS_DISP:
      fprintf(stddbg, "  Class: display     VID/DID: %4X / %4X\n", venID, devID);
      break;

    case PCI_CLASS_MULT:
      fprintf(stddbg, "  Class: multimedia  VID/DID: %4X / %4X\n", venID, devID);
      break;

    case PCI_CLASS_SERL:
      fprintf(stddbg, "  Class: serial      VID/DID: %4X / %4X\n", venID, devID);
      break;

    case PCI_CLASS_WIRL:
      fprintf(stddbg, "  Class: wireless    VID/DID: %4X / %4X\n", venID, devID);
      break;

    default:
      fprintf(stddbg, "  Class: unknown     VID/DID: %4X / %4X\n", venID, devID);
      break;
    }
  }

  if (k_storage_dev != NULL)
  {
    fprintf(stddbg, "found mass storage device\n");
    pci_describe(k_storage_dev);
  }
}
