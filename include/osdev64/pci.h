#ifndef JEP_PCI_H
#define JEP_PCI_H

#include "osdev64/axiom.h"

// Classes
// mass storage
#define PCI_CLASS_STOR  0x1
#define PCI_STOR_SCSI   0x0
#define PCI_STOR_IDE    0x1
#define PCI_STOR_FLOPPY 0x2
#define PCI_STOR_IDI    0x3
#define PCI_STOR_RAID   0x4
#define PCI_STOR_ATA    0x5
#define PCI_STOR_SATA   0x6
#define PCI_STOR_SAS    0x7

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
#define PCI_WIRL_CELL    0x40
#define PCI_WIRL_CELL_E  0x41

// unknown
#define PCI_SUBC_OTHER    0x80

// PCI device configuration space
typedef uint8_t pci_dev;

void k_pci_init();


#endif