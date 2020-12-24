#ifndef JEP_INSTRUCTOR_H
#define JEP_INSTRUCTOR_H

// Functions that use assembly instructions that don't
// have C

#include <stdint.h>


// bit masks for checking flags in control registers and RFLAGS
#define CR0_PE    ((uint64_t)1 << 0)
#define CR0_EM    ((uint64_t)1 << 1)
#define CR0_PG    ((uint64_t)1 << 31)
#define CR4_PAE   ((uint64_t)1 << 5)
#define CR4_PCIDE ((uint64_t)1 << 17)
#define CR4_SMEP  ((uint64_t)1 << 20)
#define CR4_SMAP  ((uint64_t)1 << 21)
#define CR4_PKE   ((uint64_t)1 << 22)
#define RFLAGS_CPUID ((uint64_t)1 << 21)


/**
 * Writes a byte to an I/O port.
 *
 * Params:
 *   uint16_t - an I/O port
 *   uint8_t - the byte to be written
 */
void k_outb(uint16_t port, uint8_t val);


/**
 * Reads a byte from an I/O port.
 *
 * Params:
 *   uint16_t - an I/O port
 *
 * Returns:
 *   uint8_t - a byte
 */
uint8_t k_inb(uint16_t port);


/**
 * Disables interrupts.
 */
void k_disable_interrupts();


/**
 * Enabled interrupts.
 */
void k_enable_interrupts();


/**
 * Reads the value of control register CR0.
 *
 * Returns:
 *   uint64_t - the contents of CR0
 */
uint64_t k_get_cr0();

/**
 * Writes a value into control register CR0.
 *
 * Params:
 *   uint64_t - the contents to put in CR0
 */
void k_set_cr0(uint64_t);


/**
 * Reads the value of control register CR3.
 *
 * Returns:
 *   uint64_t - the contents of CR3
 */
uint64_t k_get_cr3();


/**
 * Writes a value into control register CR3.
 *
 * Params:
 *   uint64_t - the contents to put in CR3
 */
void k_set_cr3(uint64_t);

/**
 * Reads the value of control register CR4.
 *
 * Returns:
 *   uint64_t - the contents of CR4
 */
uint64_t k_get_cr4();


/**
 * Writes a value into control register CR4.
 *
 * Params:
 *   uint64_t - the contents to put in CR4
 */
void k_set_cr4(uint64_t);


/**
 * Reads the value of control register RFLAGS.
 *
 * Returns:
 *   uint64_t - the contents of RFLAGS
 */
uint64_t k_get_rflags();


/**
 * Writes a value into RFLAGS
 *
 * Params:
 *   uint64_t - the contents to put in RFLAGS
 */
void k_set_rflags(uint64_t);


/**
 * Executes the CPUID instruction and returns the value in the RAX
 * register.
 *
 * Params:
 *   uint64_t - the input for CPUID
 *
 * Returns:
 *   uint64_t - the result of CPUID
 */
uint64_t k_cpuid_rax(uint64_t);


/**
 * Causes an exception.
 * This typically causes a divide error by attempting to divide by 0,
 * but it may be changed to do anything at any time.
 * This is mainly used to test the ISRs that handle exceptions.
 */
void k_cause_exception();

#endif