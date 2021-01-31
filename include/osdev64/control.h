#ifndef JEP_CONTROL_H
#define JEP_CONTROL_H

// Control Register Interface 
// Functions and data types for reading and writing controls registers
// CR0, CR3, CR4, and RFLAGS.


// RFLAGS Bits:
// -------------------------------------------------------------
// Bit       Abbreviation   Description
// -------------------------------------------------------------
// 0         CF             Carry Flag
// 1                        Reserved
// 2         PF             Parity Flag
// 3                        Reserved
// 4         AF             Adjust Flag
// 5                        Reserved
// 6         ZF             Zero Flag
// 7         SF             Sign Flag
// 8         TF             Trap Flag
// 9         IF             Interrupts Enabled Flag
// 10        DF             Direction Flag
// 11        OF             Overflow Flag
// [13:12]   IOPL           I/O Privilege Level
// 14        NT             Nested Task
// 15                       Reserved
// 16        RF             Resume Flag
// 17        VM             Virtual 8086 Mode
// 18        AC             Alignment Check
// 19        VIF            Virtual Interrupt Flag
// 20        VIP            Virtual Interrupt Pending
// 21        ID             CPUID Available
// -------------------------------------------------------------


// CR0 Bits:
// -------------------------------------------------------------
// Bit       Abbreviation   Description
// -------------------------------------------------------------
// 0         PE             Protected Mode Enabled
// 1         MP             Monitor Coprocessor
// 2         EM             Emulation (x87 floating point)
// 3         TS             Task Switched
// 4         ET             Extension Type
// 5         NE             Numberic Error
// 16        WP             Write Protect
// 18        AM             Alignment Mask
// 29        NW             Not Write Through
// 30        CD             Cache Disable
// 31        PG             Paging Enabled
// -------------------------------------------------------------


// CR4 Bits:
// -------------------------------------------------------------
// Bit       Abbreviation   Description
// -------------------------------------------------------------
// 0         VME            Virtual 8086 Mode Extensions
// 1         PVI            Protected Mode Virtual Interrupts
// 2         TSD            Time Stamp Disable
// 3         DE             Debugging Extensions
// 4         PSE            Page Size Extension
// 5         PAE            Physical Address Extension
// 6         MCE            Machine Check Exception
// 7         PGE            Page Global Enabled
// 8         PCE            Performance Monitoring Counter Enabled
// 9         OSFXSR         Support FXSAVE and FXRSTOR
// 10        OSXMMEXCPT     Support Unmasked SIMD Floating-Point Exceptions
// 11        UMIP           User-Mode Instruction Prevention
// 12        LA57           Level 5 paging (not used yet)
// 13        VMXE           Virtual Machine Extensions Enable
// 14        SMXE           Safer Mode Extensions Enable
// 16        FSGSBASE       RDFSBASE, RDGSBASE, WRFSBASE, and WRGSBASE
// 17        PCIDE          PCID Enable
// 18        OSXSAVE        XSAVE and Processor Extended States Enable
// 20        SMEP           Supervisor Mode Execution Protection Enable
// 21        SMAP           Supervisor Mode Access Prevention Enable
// 22        PKE            Protection Key Enable
// -------------------------------------------------------------


#include "osdev64/axiom.h"
#include "osdev64/bitmask.h"


#define CR4_PCIDE BM_17
#define CR4_PKE BM_22


/**
 * Reads the value of control register CR0.
 *
 * Returns:
 *   k_regn - the contents of CR0
 */
k_regn k_get_cr0();


/**
 * Writes a value into control register CR0.
 *
 * Params:
 *   k_regn - the contents to put in CR0
 */
void k_set_cr0(k_regn);


/**
 * Reads the value of control register CR3.
 *
 * Returns:
 *   k_regn - the contents of CR3
 */
k_regn k_get_cr3();


/**
 * Writes a value into control register CR3.
 *
 * Params:
 *   k_regn - the contents to put in CR3
 */
void k_set_cr3(k_regn);


/**
 * Reads the value of control register CR4.
 *
 * Returns:
 *   k_regn - the contents of CR4
 */
k_regn k_get_cr4();


/**
 * Writes a value into control register CR4.
 *
 * Params:
 *   k_regn - the contents to put in CR4
 */
void k_set_cr4(k_regn);


/**
 * Reads the value of control register RFLAGS.
 *
 * Returns:
 *   k_regn - the contents of RFLAGS
 */
k_regn k_get_rflags();


/**
 * Writes a value into RFLAGS
 *
 * Params:
 *   k_regn - the contents to put in RFLAGS
 */
void k_set_rflags(k_regn);

#endif