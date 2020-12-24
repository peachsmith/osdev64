#ifndef JEP_DESCRIPTOR_H
#define JEP_DESCRIPTOR_H

#include <stdint.h>

// A descriptor is a collection of bits that locate and describe something
// in memory. What each descriptor desribes could be anything from a piece
// of data, or a region of memory.
//
// Descriptors are typically gathered into collections called tables.
// The global descriptor table (GDT) contains segment descriptors which
// describe certain regions of memory.
// The interrupt descriptor table (IDT) contains descriptors that 

/**
 * A segment descriptor is 64 bits that describe a segment of memory.
 *
 * Each segment descriptor has the following structure:
 * ---------------------------------------------------------------------------
 * bits    purpose
 * ---------------------------------------------------------------------------
 * [15:0]  segment limit (16 bits)
 * [31:16] base address (16 bits)
 * [39:32] base address (8 bits)
 * [43:40] type field (contains type-specific configuration)
 * 44      type flag (0 for system, 1 for code/data)
 * [46:45] descriptor privilege level (DPL)
 * 47      presence flag
 * [51:48] segment limit (4 bits)
 * 52      available for use by software
 * 53      64-bit code segment flag (0 for compatability mode, 1 for 64-bit)
 * 54      default operation size (must be 0 when L is 1 in 64-bit mode)
 * 55      granularity (i.e. scaling) (0 for bytes, 1 for 4KB)
 * [63:56] base address (8 bits)
 * ---------------------------------------------------------------------------
 */
typedef uint64_t seg_desc;


/**
 * An interrupt gate descriptor is 128 bits that describe a procedure for
 * handling an interrupt. An interrupt gate descriptor's location in the
 * IDT indicates which interrupt is intended to trigger the function that it
 * describes.
 *
 * The type int_desc is defined as an unsigned 64-bit integer, so two of them
 * are required to build a complete 128-bit interrupt gate descriptor.
 *
 * Each interrupt gate descriptor has the following structure:
 * ---------------------------------------------------------------------------
 * bits     purpose
 * ---------------------------------------------------------------------------
 * [15:0]   address offset of interrupt handler (lower 16 bits)
 * [31:16]  segment selector offset (16 bits)
 * [34:32]  IST number (1-7 to use IST, or 0 to not use IST)
 * [39:35]  0
 * [43:40]  type configuration
 * 44       0
 * [46:45]  descriptor privilege level (DPL)
 * 47       presence flag
 * [63:48]  address offset of interrupt handler (middle 16 bits)
 * [95:64]  address offset of interrupt handler (higher 32 bits)
 * [127:96] reserved
 * ---------------------------------------------------------------------------
 */
typedef uint64_t int_desc;


/**
 * Code and Data segment descriptor type.
 *
 * For Data segment descriptors, bit 0 is the accessed flag, bit 1 is
 * the writable flag, bit 2 is the expansion direction, and 3 is 0.
 *
 * For Code segment descriptors, bit 0 is the accessed flag, bit 1 is
 * the readable flag, bit 2 is the conforming flag, and 3 is 1.
 *
 * Data Segment Descriptor Types
 * ---------------------------------------------------------------------------
 * dec  hex  bin    description
 * ---------------------------------------------------------------------------
 * 0    0x0  0000   read only
 * 1    0x1  0001   read only, accessed
 * 2    0x2  0010   read/write
 * 3    0x5  0011   read/write, accessed
 * 4    0x4  0100   read only, expand down
 * 5    0x5  0101   read only, expand down, accessed
 * 6    0x6  0110   read/write, expand down
 * 7    0x7  0111   read/write, expand down, accessed
 *
 *
 * Code Segment Descriptor Types
 * ---------------------------------------------------------------------------
 * dec  hex  bin    description
 * ---------------------------------------------------------------------------
 * 8    0x8  1000   execute only
 * 9    0x9  1001   execute only, accessed
 * 10   0xA  1010   execute/read
 * 11   0xB  1011   execute/read, accessed
 * 12   0xC  1100   execute only, conforming
 * 13   0xD  1101   execute only, conforming, accessed
 * 14   0xE  1110   execute/read, conforming
 * 15   0xF  1111   execute/read, conforming, accessed
 */
typedef uint64_t cd_seg_type;


/**
 * System segment and gate descriptor type
 * The value depends on the context and whether or not the CPU is in
 * 64-bit mode. For more information, see table 3-2 in the Intel manual
 * volume 3A part 1.
 */
typedef uint64_t sg_seg_type;


//
// data segment descriptor type constants
//

// read only
#define CD_SEG_RO   ((cd_seg_type)0x0)

// read only, accessed
#define CD_SEG_ROA  ((cd_seg_type)0x1)

// read/write
#define CD_SEG_RW   ((cd_seg_type)0x2)

// read/write, accessed
#define CD_SEG_RWA  ((cd_seg_type)0x3)

// read only, expand down
#define CD_SEG_ROD  ((cd_seg_type)0x4)

// read only, expand down, accessed
#define CD_SEG_RODA ((cd_seg_type)0x5)

// read/write, expand down
#define CD_SEG_RWD  ((cd_seg_type)0x6)

// read/write, expand down, accessed
#define CD_SEG_RWDA ((cd_seg_type)0x7)


//
// code segment descriptor type constants
//

// execute only
#define CD_SEG_EO   ((cd_seg_type)0x8)

// execute only, accessed
#define CD_SEG_EOA  ((cd_seg_type)0x9)

// execute/read
#define CD_SEG_ER   ((cd_seg_type)0xA)

// execute/read, accessed
#define CD_SEG_ERA  ((cd_seg_type)0xB)

// execute only, conforming
#define CD_SEG_EOC  ((cd_seg_type)0xC)

// execute only, conforming, accessed
#define CD_SEG_EOCA ((cd_seg_type)0xD)

// execute/read, conforming
#define CD_SEG_ERC  ((cd_seg_type)0xE)

// execute/read, conforming, accessed
#define CD_SEG_ERCA ((cd_seg_type)0xF)


//
// system segment and gate descriptor type constants
//

// TSS (available)
#define SG_SEG_TSS_AVL ((sg_seg_type)0x9)

// TSS (busy)
#define SG_SEG_TSS_BSY ((sg_seg_type)0xB)

// interrupt gate
#define SG_SEG_INT_GATE ((sg_seg_type)0xE)


/**
 * Populates a GDT and loads it into the GDTR register.
 * This function also established a TSS and loads its GDT offset into
 * the TR register.
 * The GDT created by this function contains the following descriptors:
 * 1. null descriptor (offset 0x00)
 * 2. code descriptor (offset 0x08)
 * 3. data descriptor (offset 0x10)
 * 4. TSS descriptor  (offset 0x18)
 */
void k_load_gdt();

/**
 * Populates an IDT and loads it into the IDTR register.
 * The IDT initialized by this function should contain descriptors for
 * functions that handle the first 32 interrupts (INT0 through INT31).
 */
void k_load_idt();

#endif