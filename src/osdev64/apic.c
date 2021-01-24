#include "osdev64/apic.h"
#include "osdev64/bitmask.h"
#include "osdev64/acpi.h"
#include "osdev64/paging.h"
#include "osdev64/uefi.h"
#include "osdev64/msr.h"
#include "osdev64/interrupts.h"

#include "klibc/stdio.h"

// Local APIC register address offsets.
#define LAPIC_ID ((uint64_t)0x20)
#define LAPIC_VERSION ((uint64_t)0x30)
#define LAPIC_SPURIOUS ((uint64_t)0xF0)
#define LAPIC_ISR0 ((uint64_t)0x100)
#define LAPIC_ISR1 ((uint64_t)0x110)
#define LAPIC_ISR2 ((uint64_t)0x120)
#define LAPIC_ISR3 ((uint64_t)0x130)
#define LAPIC_ISR4 ((uint64_t)0x140)
#define LAPIC_ISR5 ((uint64_t)0x150)
#define LAPIC_ISR6 ((uint64_t)0x160)
#define LAPIC_ISR7 ((uint64_t)0x170)

// IOAPIC registers
#define IOAPICVER ((uint32_t)1)

// Multiple APIC Descriptor Table (MADT)
extern unsigned char* g_madt;

// Local APIC physical base
static uint64_t g_lapic_phys;

// Local APIC Base
volatile uint32_t* volatile g_lapic = NULL;

// IO APIC base
volatile uint32_t* volatile g_ioapic = NULL;




//============================================================
// Functions for reading and writing APIC registers
//============================================================

/**
 * Reads a value from a local APIC register.
 *
 * Params:
 *   uint32_t - the offset of the register from the base address
 *
 * Returns:
 *   uint32_t - the contents of the register
 */
static uint32_t lapic_read(uint32_t offset)
{
  volatile uint32_t* volatile reg;

  reg = (volatile uint32_t*)((uint64_t)g_lapic + offset);

  return *reg;
}


/**
 * Writes a value to a local APIC register.
 *
 * Params:
 *   uint32_t - the offset of the register from the base address
 *   uint32_t - the contents to write to the register
 *
 * Returns:
 *   uint32_t - the contents of the register
 */
static uint32_t lapic_write(uint32_t offset, uint32_t value)
{
  volatile uint32_t* volatile reg;

  reg = (volatile uint32_t*)((uint64_t)g_lapic + offset);

  *reg = value;
}


/**
 * Reads a value from an IOAPIC register.
 *
 * Params:
 *   uint32_t - the index of the register
 *
 * Returns:
 *   uint32_t - the contents of the register
 */
static uint32_t ioapic_read(uint32_t index)
{
  volatile uint32_t* volatile iowin;

  iowin = (volatile uint32_t*)((uint64_t)g_ioapic + 0x10);

  *g_ioapic = index;

  return *iowin;
}


/**
 * Writes a value to an IOAPIC register.
 *
 * Params:
 *   uint32_t - the index of the register
 *   uint32_t - the contents to write to the register
 *
 * Returns:
 *   uint32_t - the contents of the register
 */
static uint32_t ioapic_write(uint32_t index, uint32_t value)
{
  volatile uint32_t* volatile iowin;

  iowin = (volatile uint32_t*)((uint64_t)g_ioapic + 0x10);

  *g_ioapic = index;

  *iowin = value;
}




//============================================================
// Interrupt handler functions
//============================================================

void apic_generic_isr();
void apic_spurious_isr();
void apic_pit_irq();
void apic_generic_legacy_irq();

void apic_generic_handler()
{
  fprintf(stddbg, "[INT] APIC generic interrupt handler\n");

  uint32_t isr0 = lapic_read(LAPIC_ISR0);
  uint32_t isr1 = lapic_read(LAPIC_ISR1);
  uint32_t isr2 = lapic_read(LAPIC_ISR2);
  uint32_t isr3 = lapic_read(LAPIC_ISR3);
  uint32_t isr4 = lapic_read(LAPIC_ISR4);
  uint32_t isr5 = lapic_read(LAPIC_ISR5);
  uint32_t isr6 = lapic_read(LAPIC_ISR6);
  uint32_t isr7 = lapic_read(LAPIC_ISR7);

  fprintf(stddbg, "ISR0: ");
  for (int i = 0; i < 32; i++)
  {
    fputc((isr0 & (1 << i)) ? '1' : '0', stddbg);
  }
  fputc('\n', stddbg);

  fprintf(stddbg, "ISR1: ");
  for (int i = 0; i < 32; i++)
  {
    fputc((isr1 & (1 << i)) ? '1' : '0', stddbg);
  }
  fputc('\n', stddbg);

  fprintf(stddbg, "ISR2: ");
  for (int i = 0; i < 32; i++)
  {
    fputc((isr2 & (1 << i)) ? '1' : '0', stddbg);
  }
  fputc('\n', stddbg);

  fprintf(stddbg, "ISR3: ");
  for (int i = 0; i < 32; i++)
  {
    fputc((isr3 & (1 << i)) ? '1' : '0', stddbg);
  }
  fputc('\n', stddbg);

  fprintf(stddbg, "ISR4: ");
  for (int i = 0; i < 32; i++)
  {
    fputc((isr4 & (1 << i)) ? '1' : '0', stddbg);
  }
  fputc('\n', stddbg);

  fprintf(stddbg, "ISR5: ");
  for (int i = 0; i < 32; i++)
  {
    fputc((isr5 & (1 << i)) ? '1' : '0', stddbg);
  }
  fputc('\n', stddbg);

  fprintf(stddbg, "ISR6: ");
  for (int i = 0; i < 32; i++)
  {
    fputc((isr6 & (1 << i)) ? '1' : '0', stddbg);
  }
  fputc('\n', stddbg);

  fprintf(stddbg, "ISR7: ");
  for (int i = 0; i < 32; i++)
  {
    fputc((isr7 & (1 << i)) ? '1' : '0', stddbg);
  }
  fputc('\n', stddbg);
}

void apic_spurious_handler()
{
  fprintf(stddbg, "[INT] APIC spurious interrupt handler\n");
}

void apic_pit_handler()
{
  fprintf(stddbg, "[INT] APIC PIT IRQ handler\n");
}

void apic_generic_legacy_handler()
{
  fprintf(stddbg, "[INT] APIC generic legacy IRQ handler\n");

  uint32_t isr0 = lapic_read(LAPIC_ISR0);
  uint32_t isr1 = lapic_read(LAPIC_ISR1);

  fprintf(stddbg, "ISR0: ");
  for (int i = 0; i < 32; i++)
  {
    fputc((isr0 & (0x80000000 >> i)) ? '1' : '0', stddbg);
  }
  fputc('\n', stddbg);

  fprintf(stddbg, "ISR1: ");
  for (int i = 0; i < 32; i++)
  {
    fputc((isr1 & (0x80000000 >> i)) ? '1' : '0', stddbg);
  }
  fputc('\n', stddbg);

  fprintf(stddbg, "TEST: ");
  uint32_t toot = ((1 << 3) | (1 << 10));
  for (int i = 0; i < 32; i++)
  {
    fputc((toot & (0x80000000 >> i)) ? '1' : '0', stddbg);
  }
  fputc('\n', stddbg);
}


void k_apic_init()
{
  if (g_madt == NULL)
  {
    printf("[MADT] No MADT detected\n");
    return;
  }

  uint64_t lapic_base = 0;
  uint64_t ioapic_base = 0;

  uint32_t table_len = *(uint32_t*)(g_madt + 4);
  uint32_t lapic32 = *(uint32_t*)(g_madt + 36);
  uint32_t flags = *(uint32_t*)(g_madt + 40);

  // Set the local APIC base to be the value we found before the
  // MADT entries list.
  lapic_base = (uint64_t)lapic32;

  // Calculate the checksum.
  unsigned char madt_check = 0;
  for (uint32_t i = 0; i < table_len; i++)
  {
    madt_check += g_madt[i];
  }

  // Read the MADT to find the physical addresses of the APICs.
  // The index i represents the byte offset from the start of the
  // entry list.
  // The entry list starts at offset 44.
  for (uint32_t i = 0; i < table_len - 44;)
  {
    unsigned char* entry = &g_madt[44 + i];

    uint8_t entry_type = *(uint8_t*)(entry);
    uint8_t entry_len = *(uint8_t*)(entry + 1);

    switch (entry_type)
    {
    case 0: // Local APIC
      break;

    case 1:
    {
      // I/O APIC
      uint32_t ioapic32 = *(uint32_t*)(entry + 4);
      uint32_t gsi = *(uint32_t*)(entry + 8);

      // For now, we only care about the I/O APIC that handles
      // the first few IRQ. sO we check that the GSI is 0.
      if (gsi == 0)
      {
        ioapic_base = (uint64_t)ioapic32;
      }
    }

    break;

    case 2: // Interrupt Source Override
    case 4: // Non-Maskable Interrupts
      break;

    case 5:
    {
      // Local APIC Address Override
      lapic_base = *(uint64_t*)(entry + 4);
    }
    break;

    default: // Unknown entry type
      break;
    }

    i += entry_len;
  }


  // Ensure that we found the base addresses of the local and IO APICs.
  if (lapic_base == 0)
  {
    printf("[ERROR] could not find local APIC physical base\n");
    return;
  }

  if (ioapic_base == 0)
  {
    printf("[ERROR] could not find I/O APIC physical base\n");
    return;
  }

  // Save the physical address of the local APIC so we can
  // put it in the IA32_APIC_BASE MSR.
  g_lapic_phys = lapic_base;

  // Map the local APIC into virtual address space.
  uint64_t lapic_virt = k_paging_map_range(lapic_base, lapic_base + 0x3F0);
  if (lapic_virt == 0)
  {
    printf("[ERROR] failed to map local APIC virtual base\n");
    for (;;);
  }
  g_lapic = (volatile uint32_t*)lapic_virt;

  // Map the I/O APIC into virtual address space.
  uint64_t ioapic_virt = k_paging_map_range(ioapic_base, ioapic_base + 0x3F0);
  if (ioapic_virt == 0)
  {
    printf("[ERROR] failed to map IO APIC virtual base\n");
    for (;;);
  }
  g_ioapic = (volatile uint32_t*)ioapic_virt;
}


int k_apic_available()
{
  return (g_lapic == NULL || g_ioapic == NULL) ? 0 : 1;
}



//============================================================
// Local APIC
//============================================================

void k_lapic_enable()
{
  // Install the APIC version of the generic ISRs.
  for (int i = 0x30; i < 0xFF; i++)
  {
    k_install_isr((uint64_t)apic_generic_isr, i);
  }

  // Install the spurious interrupt handler at index 255;
  k_install_isr((uint64_t)apic_spurious_isr, 0xFF);

  // Ensure that the physical address of the local APIC
  // is in the IA32_APIC_BASE MSR.
  uint64_t apic_base = k_msr_get(IA32_APIC_BASE);
  if ((apic_base &= ~(BM_12_BITS)) != g_lapic_phys)
  {
    apic_base |= g_lapic_phys;
    k_msr_set(IA32_APIC_BASE, apic_base);
  }

  // Set bit 8 in the spurious interrupt vector register,
  // and set bits [7:0] to be the index of the spurious interrupt
  // handler in the IDT.
  uint32_t spurious = lapic_read(LAPIC_SPURIOUS);

  // According to the Intel manual, when bits [3:0] are hardwired,
  // software writes to those bits have no effect.
  spurious &= ~(0xFF);
  spurious |= 0xFF;

  lapic_write(LAPIC_SPURIOUS, spurious | 0x100);
}


uint32_t k_lapic_get_id()
{
  // The ID is located in bits [31:24] of the ID register.
  uint32_t id = lapic_read(LAPIC_ID);
  return (id & 0xFF000000) >> 24;
}


uint32_t k_lapic_get_version()
{
  // The max LVT is located in bits [7:0] of the version register.
  uint32_t version = lapic_read(LAPIC_VERSION);
  return version & 0xFF;
}


uint32_t k_lapic_get_maxlvt()
{
  // The max LVT is located in bits [23:16] of the version register.
  uint32_t version = lapic_read(LAPIC_VERSION);
  return (version & 0xFF0000) >> 16;
}



//============================================================
// I/O APIC
//============================================================

/**
 * Sets an IRQ redirect for the current I/O APIC.
 *
 * Params:
 *   uint8_t - the IRQ number to redirect
 *   uint8_t - the index of the ISR in the IDT
 */
static void ioapic_set_irq(uint8_t irq, uint8_t isr)
{
  uint32_t lo_index = irq * 2;
  uint32_t hi_index = irq * 2 + 1;

  // Get the current APIC ID
  uint32_t lapic_id = k_lapic_get_id();

  // Currently, the I/O APIC registers only have 4 bits for the
  // local APIC ID.
  // In theory, some chips can have up to 255 processors.
  // TODO: figure out how to handle this scenario.
  if (lapic_id > 0xF)
  {
    fprintf(stddbg, "[ERROR] local APIC ID is more than 4 bits\n");
    for (;;);
  }


  // Put the local APIC ID in bits [59:56].
  // This is bits [31:24] of the high register.
  uint32_t hi_contents = ioapic_read(hi_index);

  // Clear bits [31:24] and set the local APIC address.
  hi_contents &= ~0xFF000000;
  hi_contents |= (lapic_id << 24);

  ioapic_write(hi_index, hi_contents);


  // Most of the IRQ description goes in the low register.
  uint32_t lo_contents = ioapic_read(lo_index);

  // Clear bits [10:8] to indicate fixed delivery mode.
  // TODO: add doc comments for delivery mode.
  lo_contents &= ~(0x700);

  // Clear bit 11 to indicate physical destination mode.
  lo_contents &= ~(0x800);

  // Bits [15:12] are ignored for now.
  // Bit 13 is the polarity. (1 for low, 0 for high)
  // Bit 15 is the trigger mode. (1 for level, 0 for edge)

  // Clear bit 16 to unmask the IRQ.
  lo_contents &= ~(0x10000);

  // Clear bits [7:0] in preparation for the new ISR index.
  lo_contents &= ~0xFF;

  // Set the index of the ISR in bits [7:0].
  lo_contents |= isr;

  ioapic_write(lo_index, lo_contents);

}

void k_ioapic_configure()
{
  // Set the IRQ redirects.
  // The PIC IRQs were mapped to ISR 0x20 through 0x2F
  // So we'll start at 0x30.
  for (uint8_t i = 0; i < 24; i++)
  {
    ioapic_set_irq(i, 0x30 + i);
    if (i < 0x10)
    {
      k_install_isr((uint64_t)apic_generic_legacy_handler, 0x30 + i);
    }
  }

  // Looks for ISOs in the MADT and install the
  // IRQs accordingly.
  uint32_t table_len = *(uint32_t*)(g_madt + 4);
  for (uint32_t i = 0; i < table_len - 44;)
  {
    unsigned char* entry = &g_madt[44 + i];

    uint8_t entry_type = *(uint8_t*)(entry);
    uint8_t entry_len = *(uint8_t*)(entry + 1);

    switch (entry_type)
    {

    case 2:
    {
      uint8_t bus_src = *((uint8_t*)(entry + 2));
      uint8_t irq_src = *((uint8_t*)(entry + 3));
      uint32_t gsi = *((uint32_t*)(entry + 4));
      // uint16_t flags = *((uint16_t*)(entry + 8));

      printf("[APIC] ISO: { Bus: %3u, IRQ: %3u, GSI: %3u }\n",
        bus_src,
        irq_src,
        gsi
      );

      // polarity and trigger mode
      // uint64_t pol = ((uint64_t)flags & BM_2_BITS);
      // uint64_t trig = ((uint64_t)flags & (BM_2_BITS << 2)) >> 2;

      // printf("[MADT] ISO: { Bus: %3u, IRQ: %3u, GSI: %3u Pol: %5s, Trig: %5s }\n",
      //   bus_src,
      //   irq_src,
      //   gsi,
      //   iso_polarity_str(pol),
      //   iso_trigger_str(trig)
      // );

      // Add a redirection entry to send but-relative IRQ0 to
      // the specified global system interrupt.
      ioapic_set_irq(irq_src, 0x30 + gsi);

      // Timer IRQ
      if (irq_src == 0)
      {
        printf("[APIC] installing the APIC PIT ISR\n");

        // Install the ISR for handling the timer IRQ.
        k_install_isr((uint64_t)apic_pit_irq, 0x30 + gsi); // 0x30 + gsi);
      }
    }
    break;

    default:
      break;
    }

    i += entry_len;
  }
}


uint32_t k_ioapic_get_version()
{
  // The I/O APIC version is located in bits [7:0]
  // of the IOAPICVER register.
  uint32_t version = ioapic_read(IOAPICVER);

  return version & 0xFF;
}


uint32_t k_ioapic_get_max_redirect()
{
  // The max redirects - 1 is located in bits [23:16]
  // of the IOAPICVER register.
  uint32_t version = ioapic_read(IOAPICVER);
  return (version & 0xFF0000) >> 16;
}
