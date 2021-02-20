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


typedef struct pci_id_map {
  uint16_t vendorID;
  uint16_t deviceID;
  char name[48];
}pci_id_map;


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



// PCI device class and subclass descriptions
//
// 0x1 Mass Storage
//  - 0x0 SCSI controller
//  - 0x1 IDE controller
//  - 0x2 Floppy controller
//  - 0x3 IPI bus controller
//  - 0x4 RAID controller 
//
// 0x2 Network Controller
//  - 0x1 Ethernet
//
// 0x3 Display Controller
//  - 0x0 VGA-compatible
//  - 0x1 XGA
//  - 0x2 3D controller
//
// 0x4 Multimedia Device
//  - 0x0 video
//  - 0x1 audio
//  - 0x2 telephony
//  - 0x3 high definition audio
//
// 0x6 Bridge Device
//  - 0x0 host bridge
//  - 0x1 ISA bridge
//  - 0x2 EISA bridge
//  - 0x3 MCA bridge
//  - 0x4 PCI-to-PCI bridge
//  - 0x5 PCMCIA bridge
//  - 0x6 NuBus bridge
//  - 0x7 CardBus bridge
//  - 0x8 RACEway bridge
//
// 0xC  Serial Bus Controller
//  - 0x0 IEEE 1394
//  - 0x1 ACCESS.bus
//  - 0x2 SSA
//  - 0x3 serial bus, host controller, or device 
//  - 0x4 Fibre channel
//  - 0x5 SMBus
//  - 0x7 IPMI
//  - 0x8 SERCOS interface
//  - 0x9 CANbus
//  - 0xA MIPI I3C host controller interface
//
// 0xD Wireless Controller
//  - 0x0 iRDA controller
//  - 0x1 consumer IR controller or UWB radio controller
//  - 0x10 RF controller
//  - 0x11 Bluetooth
//  - 0x12 Broadband
//  - 0x20 Ethernet 5 GHz
//  - 0x21 Ethernet 2.4 GHz
//  - 0x40 Cellular Modem
//  - 0x41 Cellular Modem (with Ethernet)



// Classes
// mass storage
#define PCI_CLASS_STOR  0x1
#define PCI_STOR_SCSI   0x0
#define PCI_STOR_IDE    0x1
#define PCI_STOR_FLOPPY 0x2
#define PCI_STOR_IDI    0x3
#define PCI_STOR_RAID   0x4

// network
#define PCI_CLASS_NETW    0x2
#define PCI_NETW_ETHERNET 0x1

// display
#define PCI_CLASS_DISP 0x3
#define PCI_DISP_VGA   0x0
#define PCI_DISP_XGA   0x1
#define PCI_DISP_3D    0x2

// multimedia
#define PCI_CLASS_MULT 0x4
#define PCI_MULT_VIDEO 0x0
#define PCI_MULT_AUDIO 0x1
#define PCI_MULT_PHON  0x2
#define PCI_MULT_HDAUD 0x3

// bridge
#define PCI_CLASS_BRIG   0x6
#define PCI_BRIG_HOST    0x0
#define PCI_BRIG_ISA     0x1
#define PCI_BRIG_EISA    0x2
#define PCI_BRIG_MCA     0x3
#define PCI_BRIG_PCI     0x4
#define PCI_BRIG_PCMCIA  0x5
#define PCI_BRIG_NUBUS   0x6
#define PCI_BRIG_CARDBUS 0x7
#define PCI_BRIG_RACEWAY 0x8

// serial
#define PCI_CLASS_SERL  0xC
#define PCI_SERL_IEEE   0x0
#define PCI_SERL_ACCESS 0x1
#define PCI_SERL_SSA    0x2
#define PCI_SERL_USB    0x3
#define PCI_SERL_FIBRE  0x4
#define PCI_SERL_SMB    0x5
#define PCI_SERL_IPMI   0x7
#define PCI_SERL_SERCOS 0x8
#define PCI_SERL_CANBUS 0x9
#define PCI_SERL_MIPI   0xA

// wireless
#define PCI_CLASS_WIRL   0xD
#define PCI_WIRL_IRDA    0x0
#define PCI_WIRL_IR_UWB  0x1
#define PCI_WIRL_RF      0x10
#define PCI_WIRL_BLUE    0x11
#define PCI_WIRL_BROAD   0x12
#define PCI_WIRL_ETH_5   0x20
#define PCI_WIRL_ETH_2_4 0x21
#define PCI_WIRL_CELL    0x21
#define PCI_WIRL_CELL_E  0x21

// unknown
#define PCI_SUBC_OTHER    0x80


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

    // Return 1 to indicate a multi-function device,
    // or 2 to indicate a single-function device.
    return (hea & 0x80) ? 1 : 2;
  }

  return 0;
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
