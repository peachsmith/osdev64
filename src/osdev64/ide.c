#include "osdev64/ide.h"
#include "osdev64/pci.h"
#include "osdev64/memory.h"
#include "osdev64/instructor.h"
#include "osdev64/paging.h"
#include "osdev64/pit.h"

#include "klibc/stdio.h"


// This implementation is based on the IDE controller example found
// on the osdev wiki at https://wiki.osdev.org/PCI_IDE_Controller


// The following preprocessor macros were copied directly from
// the wiki.
#define ATA_SR_BSY     0x80    // Busy
#define ATA_SR_DRDY    0x40    // Drive ready
#define ATA_SR_DF      0x20    // Drive write fault
#define ATA_SR_DSC     0x10    // Drive seek complete
#define ATA_SR_DRQ     0x08    // Data request ready
#define ATA_SR_CORR    0x04    // Corrected data
#define ATA_SR_IDX     0x02    // Index
#define ATA_SR_ERR     0x01    // Error

#define ATA_ER_BBK      0x80    // Bad block
#define ATA_ER_UNC      0x40    // Uncorrectable data
#define ATA_ER_MC       0x20    // Media changed
#define ATA_ER_IDNF     0x10    // ID mark not found
#define ATA_ER_MCR      0x08    // Media change request
#define ATA_ER_ABRT     0x04    // Command aborted
#define ATA_ER_TK0NF    0x02    // Track 0 not found
#define ATA_ER_AMNF     0x01    // No address mark

#define ATA_CMD_READ_PIO          0x20
#define ATA_CMD_READ_PIO_EXT      0x24
#define ATA_CMD_READ_DMA          0xC8
#define ATA_CMD_READ_DMA_EXT      0x25
#define ATA_CMD_WRITE_PIO         0x30
#define ATA_CMD_WRITE_PIO_EXT     0x34
#define ATA_CMD_WRITE_DMA         0xCA
#define ATA_CMD_WRITE_DMA_EXT     0x35
#define ATA_CMD_CACHE_FLUSH       0xE7
#define ATA_CMD_CACHE_FLUSH_EXT   0xEA
#define ATA_CMD_PACKET            0xA0
#define ATA_CMD_IDENTIFY_PACKET   0xA1
#define ATA_CMD_IDENTIFY          0xEC

#define ATA_IDENT_DEVICETYPE   0
#define ATA_IDENT_CYLINDERS    2
#define ATA_IDENT_HEADS        6
#define ATA_IDENT_SECTORS      12
#define ATA_IDENT_SERIAL       20
#define ATA_IDENT_MODEL        54
#define ATA_IDENT_CAPABILITIES 98
#define ATA_IDENT_FIELDVALID   106
#define ATA_IDENT_MAX_LBA      120
#define ATA_IDENT_COMMANDSETS  164
#define ATA_IDENT_MAX_LBA_EXT  200

#define IDE_ATA        0x00
#define IDE_ATAPI      0x01

#define ATA_MASTER     0x00
#define ATA_SLAVE      0x01

#define ATA_REG_DATA       0x00
#define ATA_REG_ERROR      0x01
#define ATA_REG_FEATURES   0x01
#define ATA_REG_SECCOUNT0  0x02
#define ATA_REG_LBA0       0x03
#define ATA_REG_LBA1       0x04
#define ATA_REG_LBA2       0x05
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_COMMAND    0x07
#define ATA_REG_STATUS     0x07
#define ATA_REG_SECCOUNT1  0x08
#define ATA_REG_LBA3       0x09
#define ATA_REG_LBA4       0x0A
#define ATA_REG_LBA5       0x0B
#define ATA_REG_CONTROL    0x0C
#define ATA_REG_ALTSTATUS  0x0C
#define ATA_REG_DEVADDRESS 0x0D

// Channels:
#define      ATA_PRIMARY      0x00
#define      ATA_SECONDARY    0x01

// Directions:
#define      ATA_READ      0x00
#define      ATA_WRITE     0x01


typedef struct ide_cfg {
  uint16_t io;
  uint16_t ctrl;
  uint16_t bmide;
}ide_cfg;


// mass storage device from PCI interface.
extern pci_dev* k_storage_dev;

// identification space
static k_byte* ident_buffer;

// indicates whether an IRQ was raised
static int irq_raised;

// ATAPI packet
static k_byte* atapi_packet;

// controller port configuration
static ide_cfg cfg[2];


static void ide_write(uint8_t channel, uint8_t reg, uint8_t data);
static uint8_t ide_read(uint8_t channel, uint8_t reg);
static void ide_read_buffer(uint8_t channel, uint8_t reg, k_byte* buffer);

void k_ide_init()
{
  if (k_storage_dev == NULL)
  {
    fprintf(stddbg, "no mass storage device detected\n");
    return;
  }

  uint16_t venID = *(uint16_t*)k_storage_dev;       // vendor ID
  uint16_t devID = *(uint16_t*)(k_storage_dev + 2); // device ID
  uint8_t pif = *(uint8_t*)(k_storage_dev + 9);     // programming
  uint8_t sub = *(uint8_t*)(k_storage_dev + 10);    // subclass
  uint8_t cls = *(uint8_t*)(k_storage_dev + 11);    // class
  uint8_t hea = *(uint8_t*)(k_storage_dev + 14);    // header type

  // BAR0: primary channel I/O base (8 ports)
  // BAR1: primary channel control base (4 ports)
  // BAR2: secondary channel I/O base (8 ports)
  // BAR3: secondary channel control base (4 ports)
  // BAR4: Bus master IDE (16 ports, 8 for each channel) 
  uint32_t bar0 = *(uint32_t*)(k_storage_dev + 16);
  uint32_t bar1 = *(uint32_t*)(k_storage_dev + 20);
  uint32_t bar2 = *(uint32_t*)(k_storage_dev + 24);
  uint32_t bar3 = *(uint32_t*)(k_storage_dev + 28);
  uint32_t bar4 = *(uint32_t*)(k_storage_dev + 32);

  // Check the subclass to verify that the device is an IDE controller.
  if (sub != PCI_STOR_IDE)
  {
    fprintf(stddbg, "mass storage device is not an IDE controller\n");
    return;
  }

  fprintf(stddbg, "found an IDE controller. PIF: %X\n", pif);

  // Bit                 Meaning
  // 0   1: PCI native,          0: compatibility (primary)
  // 1   1: bit 0 is read/write  0: bit 0 is read only
  // 2   1: PCI native,          0: compatibility (secondary)
  // 3   1: bit 2 is read/write  0: bit 2 is read only
  // 7   1: bus master IDE,      0: DMA is not supported

  // Primary Channel Compatibility Mode:
  // ports 0x1F0-0x1F7, 0x3F6, IRQ14

  // Secondary Channel Compatibility Mode:
  // ports 0x170-0x177, 0x376, IRQ15

  // ide_cfg cfg[2];
  cfg[ATA_PRIMARY].io = 0x1F0;
  cfg[ATA_PRIMARY].ctrl = 0x3F6;

  cfg[ATA_SECONDARY].io = 0x170;
  cfg[ATA_SECONDARY].ctrl = 0x376;

  // If bit 0 is set, then the primary channel is in PCI native
  // mode, and we should get the ports from the BAR0 and BAR1.
  if (pif & 0x1)
  {
    cfg[ATA_PRIMARY].io = bar0 & 0xFFFF;
    cfg[ATA_PRIMARY].ctrl = (bar1 & 0xFFFF) + 2; // Only use offset 2
  }

  // If bit 2 is set, then the secondary channel is in PCI native
  // mode, and we should get the ports from the BAR2 and BAR3.
  if (pif & 0x4)
  {
    cfg[ATA_SECONDARY].io = bar2 & 0xFFFF;
    cfg[ATA_SECONDARY].ctrl = (bar3 & 0xFFFF) + 2; // Only use offset 2
  }

  // If bit 7 is set, get the bus master ports.
  if (pif & 0x80)
  {
    cfg[ATA_PRIMARY].bmide = bar4;
    cfg[ATA_SECONDARY].bmide = bar4 + 8;
  }

  fprintf(
    stddbg,
    "IDE Config:\nPIO: %X\nPCTRL: %X\nSIO: %X\nSCTRL: %X\n",
    cfg[ATA_PRIMARY].io,
    cfg[ATA_PRIMARY].ctrl,
    cfg[ATA_SECONDARY].io,
    cfg[ATA_SECONDARY].ctrl
  );

  // The identification buffer will be 2048 bytes.
  ident_buffer = (k_byte*)k_memory_alloc_pages(1);
  if (ident_buffer == NULL)
  {
    fprintf(stddbg, "failed to allocate memory for IDE identification buffer\n");
    return;
  }

  // The ATAPI packet buffer will be immediately after the
  // identification buffer in the page.
  atapi_packet = ident_buffer + 2048;


  // Disable IRQs
  ide_write(ATA_PRIMARY, ATA_REG_CONTROL, 2);
  ide_write(ATA_SECONDARY, ATA_REG_CONTROL, 2);

  // Check for drives.
  for (uint8_t i = 0; i < 2; i++)
  {
    for (uint8_t j = 0; j < 2; j++)
    {
      fprintf(stddbg, "BEGIN checking drive %d on %s channel\n", j, i == ATA_PRIMARY ? "PRIMARY" : "SECONDARY");

      uint8_t err;
      uint8_t type;
      uint8_t status;

      // Select a drive.
      ide_write(i, IDE_ATA, 0xA0 | (j << 4));
      k_pit_wait(10);

      // Identify the drive.
      ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
      k_pit_wait(10);

      // for (int polling = 1; polling && polling < 1000;)
      for (int polling = 1; polling;)
      {
        status = ide_read(i, ATA_REG_STATUS);

        if (status & ATA_SR_ERR)
        {
          polling = 0;
          err = 1;
        }
        else if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ))
        {
          polling = 0;
        }
        else
        {
          polling++;
          if (polling >= 1000)
          {
            fprintf(stddbg, "[ERROR] failed to find a drive\n");
            return;
          }
        }
      }

      // Check for ATAPI.
      if (err)
      {
        fprintf(stddbg, "checking for ATAPI\n");
        uint8_t cl = ide_read(i, ATA_REG_LBA1);
        uint8_t ch = ide_read(i, ATA_REG_LBA2);

        if ((cl == 0x14 && ch == 0xEB)
          || (cl == 0x69 && ch == 0x96))
        {
          type = IDE_ATAPI;
          ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
          k_pit_wait(10);

          // Clear the error.
          err = 0;
        }
      }

      if (err)
      {
        if (err)
        {
          if (status & ATA_ER_AMNF) { fprintf(stddbg, "[ERROR] No Address Mark Found\n"); }
          else if (status & ATA_ER_TK0NF) { fprintf(stddbg, "[ERROR] Track 0 not found\n"); }
          else if (status & ATA_ER_ABRT) { fprintf(stddbg, "[ERROR] Command Aborted\n"); }
          else if (status & ATA_ER_MCR) { fprintf(stddbg, "[ERROR] Media change request\n"); }
          else if (status & ATA_ER_IDNF) { fprintf(stddbg, "[ERROR] ID mark not Found\n"); }
          else if (status & ATA_ER_MC) { fprintf(stddbg, "[ERROR] Media changed\n"); }
          else if (status & ATA_ER_UNC) { fprintf(stddbg, "[ERROR] Uncorrectable Data Error\n"); }
          else if (status & ATA_ER_BBK) { fprintf(stddbg, "[ERROR] Bad Sectors\n"); }
          else { fprintf(stddbg, "[ERROR] Unknown error\n"); }
        }
      }
      else
      {
        fprintf(stddbg, "reading identification buffer\n");
        ide_read_buffer(i, ATA_REG_DATA, ident_buffer);

        char* sig = (char*)(ident_buffer + 54);

        for (int k = 0; k < 40; k++)
        {
          fputc(sig[k], stddbg);
        }
        fputc('\n', stddbg);
      }

      fprintf(stddbg, "END checking drive %d on %s channel\n", j, i == ATA_PRIMARY ? "PRIMARY" : "SECONDARY");
    }
  }
}


// NOTE: for now, the IRQ disable bit will be 1.

static void ide_write(uint8_t channel, uint8_t reg, uint8_t data)
{
  if (reg > 0x7 && reg < 0xC)
  {
    ide_write(channel, ATA_REG_CONTROL, 0x80 | 1);
  }

  if (reg < 0x8)
  {
    k_outb(cfg[channel].io + reg, data);
  }
  else if (reg < 0xC)
  {
    k_outb(cfg[channel].io + reg - 6, data);
  }
  else if (reg < 0xE)
  {
    k_outb(cfg[channel].ctrl + reg - 10, data);
  }
  else if (reg < 0x16)
  {
    k_outb(cfg[channel].bmide + reg - 14, data);
  }

  if (reg > 0x7 && reg < 0xC)
  {
    ide_write(channel, ATA_REG_CONTROL, 0x80 | 1);
  }
}

static uint8_t ide_read(uint8_t channel, uint8_t reg)
{
  uint8_t result = 0;

  if (reg > 0x7 && reg < 0xC)
  {
    ide_write(channel, ATA_REG_CONTROL, 0x80 | 1);
  }

  if (reg < 0x8)
  {
    result = k_inb(cfg[channel].io + reg);
  }
  else if (reg < 0xC)
  {
    result = k_inb(cfg[channel].io + reg - 6);
  }
  else if (reg < 0xE)
  {
    result = k_inb(cfg[channel].ctrl + reg - 10);
  }
  else if (reg < 0x16)
  {
    result = k_inb(cfg[channel].bmide + reg - 14);
  }

  if (reg > 0x7 && reg < 0xC)
  {
    ide_write(channel, ATA_REG_CONTROL, 0x80 | 1);
  }

  return result;
}

static void ide_read_buffer(uint8_t channel, uint8_t reg, k_byte* buffer)
{
  if (reg > 0x7 && reg < 0xC)
  {
    ide_write(channel, ATA_REG_CONTROL, 0x80 | 1);
  }

  if (reg < 0x8)
  {
    for (int i = 0; i < 256; i++)
    {
      uint16_t result = k_inw(cfg[channel].io + reg);
      buffer[i * 2] = result & 0xFF;
      buffer[i * 2 + 1] = (result & 0xFF00) >> 8;
    }
  }
  else if (reg < 0xC)
  {
    for (int i = 0; i < 256; i++)
    {
      uint16_t result = k_inw(cfg[channel].io + reg - 6);
      buffer[i * 2] = result & 0xFF;
      buffer[i * 2 + 1] = (result & 0xFF00) >> 8;
    }
  }
  else if (reg < 0xE)
  {
    // result = k_inb(cfg[channel].ctrl + reg - 10);
    for (int i = 0; i < 256; i++)
    {
      uint16_t result = k_inw(cfg[channel].ctrl + reg - 10);
      buffer[i * 2] = result & 0xFF;
      buffer[i * 2 + 1] = (result & 0xFF00) >> 8;
    }
  }
  else if (reg < 0x16)
  {
    // result = k_inb(cfg[channel].bmide + reg - 14);
    for (int i = 0; i < 256; i++)
    {
      uint16_t result = k_inw(cfg[channel].bmide + reg - 14);
      buffer[i * 2] = result & 0xFF;
      buffer[i * 2 + 1] = (result & 0xFF00) >> 8;
    }
  }

  if (reg > 0x7 && reg < 0xC)
  {
    ide_write(channel, ATA_REG_CONTROL, 0x80 | 1);
  }
}
