#include "osdev64/pic.h"
#include "osdev64/instructor.h"
#include "osdev64/bitmask.h"
#include "osdev64/interrupts.h"

#include "klibc/stdio.h"

#include <stdint.h>

#define MASTER_COM ((uint16_t)0x20)
#define MASTER_DAT ((uint16_t)0x21)
#define SLAVE_COM ((uint16_t)0xA0)
#define SLAVE_DAT ((uint16_t)0xA1)

#define PIC_EOI 0x20

#define ICW1_ICW4      ((uint16_t)0x01) /* ICW4 (not) needed */
#define ICW1_SINGLE    ((uint16_t)0x02) /* Single (cascade) mode */
#define ICW1_INTERVAL4 ((uint16_t)0x04) /* Call address interval 4 (8) */
#define ICW1_LEVEL     ((uint16_t)0x08) /* Level triggered (edge) mode */
#define ICW1_INIT      ((uint16_t)0x10) /* Initialization - required! */

#define ICW4_8086       ((uint16_t)0x01) /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO       ((uint16_t)0x02) /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE  ((uint16_t)0x08) /* Buffered mode/slave */
#define ICW4_BUF_MASTER ((uint16_t)0x0C) /* Buffered mode/master */
#define ICW4_SFNM       ((uint16_t)0x10) /* Special fully nested (not) */

#define READ_IRR ((uint16_t)0x0A) /* OCW3 irq ready next CMD read */
#define READ_ISR ((uint16_t)0x0B) /* OCW3 irq service next CMD read */


// PIC IRQ handlers
void pic_irq_0();
void pic_irq_1();
void pic_irq_2();
void pic_irq_3();
void pic_irq_4();
void pic_irq_5();
void pic_irq_6();
void pic_irq_7();
void pic_irq_8();
void pic_irq_9();
void pic_irq_10();
void pic_irq_11();
void pic_irq_12();
void pic_irq_13();
void pic_irq_14();
void pic_irq_15();


void k_pic_init()
{
  // This PIC initialization implementation is based on the example
  // found on the OSDev wiki at https://wiki.osdev.org/8259_PIC
  // It remaps the PIC IRQs to interrupts 32 through 47.


  // First, we install the PIC IRQ handlers
  k_install_isr(pic_irq_0, 32);
  k_install_isr(pic_irq_1, 33);
  k_install_isr(pic_irq_2, 34);
  k_install_isr(pic_irq_3, 35);
  k_install_isr(pic_irq_4, 36);
  k_install_isr(pic_irq_5, 37);
  k_install_isr(pic_irq_6, 38);
  k_install_isr(pic_irq_7, 39);
  k_install_isr(pic_irq_8, 40);
  k_install_isr(pic_irq_9, 41);
  k_install_isr(pic_irq_10, 42);
  k_install_isr(pic_irq_11, 43);
  k_install_isr(pic_irq_12, 44);
  k_install_isr(pic_irq_13, 45);
  k_install_isr(pic_irq_14, 46);
  k_install_isr(pic_irq_15, 47);


  // Get the masks. (probably unnecessary)
  // uint8_t master_mask = k_inb(MASTER_DAT);
  // uint8_t slave_mask = k_inb(SLAVE_DAT);

  // Start the PIC initialization sequence.
  k_outb(MASTER_COM, ICW1_INIT | ICW1_ICW4);
  k_outb(SLAVE_COM, ICW1_INIT | ICW1_ICW4);

  // set the IRQ offset to start at 32 for the master PIC
  // and 40 for the slave PIC.
  k_outb(MASTER_DAT, 0x20); // ICW2: notify master PIC of offset at index 32
  k_outb(SLAVE_DAT, 0x28);  // ICW2: notify slave PIC of offset at index 40

  k_outb(MASTER_DAT, 4); // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
  k_outb(SLAVE_DAT, 2);  // ICW3: tell Slave PIC its cascade identity (0000 0010)

  k_outb(MASTER_DAT, ICW4_8086);
  k_outb(SLAVE_DAT, ICW4_8086);

  // Restore the masks.
  // NOTE: this appears to be unnecessary. In fact, masking the interrupts
  // prevents them from being handled.
  // k_outb(MASTER_DAT, master_mask);
  // k_outb(SLAVE_DAT, slave_mask);
}


void k_pic_disable()
{
  k_outb(SLAVE_DAT, 0xFF);
  k_outb(MASTER_DAT, 0xFF);
}

uint64_t g_ticks = 0;

void k_pic_send_eoi(uint8_t irq)
{
  uint16_t isrs = k_pic_get_isr();

  uint8_t master_isr = (isrs & 0xFF);
  uint8_t slave_isr = (isrs & 0xFF) >> 8;

  fprintf(stddbg,
    "[PIC] IRQ: %u, Master ISR: %u, Slave ISR: %u\n",
    irq,
    master_isr,
    slave_isr
  );

  // Check for spurious interrupts.
  // If the interrupt is IRQ7, then bit 7 of the master PIC's
  // ISR should be set. If it's not set, then don't send an EOI.
  if (irq == 7)
  {
    if (!(master_isr & BM_7))
    {
      return;
    }
  }

  // If we receive a spurious interrupt from the slave PIC, send
  // an EOI to the master PIC but not the slave PIC.
  if (irq == 15)
  {
    if (!(slave_isr & BM_7))
    {
      k_outb(MASTER_COM, PIC_EOI);
      return;
    }
  }

  // handle PS/2 keyboard interrupts
  // TODO: figure out what's actually going on here.
  // Currently, all I can find is forum posts where
  // people just say "read from port 0x60".
  if (irq == 1)
  {
    uint8_t kb = k_inb(0x60);
    fprintf(stddbg, "read %X from keyboard\n", kb);
  }


  // If the IRQ number is greater than 7, then we need to
  // send the EOI to the slave PIC in addition to the master PIC.
  if (irq > 7)
  {
    k_outb(SLAVE_COM, PIC_EOI);
  }

  // Send the EOI to the master PIC.
  k_outb(MASTER_COM, PIC_EOI);
}

static inline uint16_t pic_read_irq_reg(uint8_t ocw3)
{
  k_outb(MASTER_COM, ocw3);
  k_outb(SLAVE_COM, ocw3);

  uint8_t slave_reg = k_inb(SLAVE_COM);
  uint8_t master_reg = k_inb(MASTER_COM);

  return ((uint16_t)slave_reg << 8 | master_reg);
}


uint16_t k_pic_get_irr()
{
  return pic_read_irq_reg(READ_IRR);
}


uint16_t k_pic_get_isr()
{
  return pic_read_irq_reg(READ_ISR);
}

