#ifndef JEP_PIC_H
#define JEP_PIC_H


#include <stdint.h>


/**
 * Initializes the PIC or emulation thereof.
 */
void k_pic_init();


/**
 * Disables the PIC.
 * This should be called if APICs are used.
 */
void k_pic_disable();


/**
 * Sends the end of interrupt (EOI) to the PIC.
 *
 * Params:
 *   uint8_t - the IRQ number ranging from 0 to 15
 */
void k_pic_send_eoi(uint8_t);


/**
 * Reads the contents of the PIC's IRR.
 * The low 8 bits are the IRR of the master PIC and the
 * high 8 bits are the IRR of the slave PIC.
 *
 * Returns:
 *   uint16_t - the values of the IRR
 */
uint16_t k_pic_get_irr();


/**
 * Reads the contents of the PIC's ISR.
 * The low 8 bits are the IsR of the master PIC and the
 * high 8 bits are the ISR of the slave PIC.
 *
 * Returns:
 *   uint16_t - the values of the ISR
 */
uint16_t k_pic_get_isr();

#endif