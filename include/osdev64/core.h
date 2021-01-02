#ifndef JEP_CORE_H
#define JEP_CORE_H

// Global definitions used by various interfaces.


// maximum number of RAM pool entries
#define RAM_POOL_MAX 32


// maximum number of RAM ledger entries
#define RAM_LEDGER_MAX 1000

// maximum number of virtual address map ledger entries
#define MAP_LEDGER_MAX 1000


// the number of 64-bit entries in the GDT
#define GDT_COUNT 5


// number of 32-bit entries in the TSS
#define TSS_COUNT 26


// The initial number of descriptors in the IDT multiplied
// by 2, since each descriptor is 128 bits.
#define IDT_COUNT 64


// console dimensions in pixels
#define CONSOLE_WIDTH 640
#define CONSOLE_HEIGHT 480


// glyph dimensions in pixels
#define GLYPH_WIDTH 8
#define GLYPH_HEIGHT 16

#endif