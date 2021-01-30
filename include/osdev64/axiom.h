#ifndef JEP_AXIOM_H
#define JEP_AXIOM_H

// This file contains assumptions about the environment and target
// architecture.

//--------------------------------
// Target Architecture Assumptions
//--------------------------------
// 1. The target architecture is x86_64.
// 2. The target architecture has 64-bit general purpose registers.
// 3. The bits of a physical or virtual address are less than or
//    equal to 64.


//-----------------------
// Build Tool Assumptions
//-----------------------
// 1. The compiler toolchain is GCC built without red zone.
// 2. The compiler toolchain supports at least the C99 standard with stdint.h.
// 3. The compiler toolchain outputs 64-bit code that uses the System V ABI.
// 4. The compiler toolchain will link against GNU-EFI for building UEFI.
//    applications.
// 5. A pointer to one data type can be freely cast to a pointer to another
//    data type type as long as the contents of the memory in question are
//    known or trivial.


//---------------------
// Firmware Assumptions
//---------------------
// 1. The firmware will comply with the UEFI specification, which, at the time
//    of writing this, can be found at https://www.uefi.org/specifications/.



// This includes definitions for the following types:
// uint64_t
// uint32_t
// uint16_t
// uint8_t
#include <stdint.h>


// UEFI functions and data types, including, but not limited to:
// NULL - a macro that exapnds to an expression of a pointer with a 0 value
// EFI_HANDLE - a pointer to something
// EFI_SYSTEM_TABLE - the interface between the OS and the UEFI services
#include <efi.h>
#include <efilib.h>

// k_byte represents a single byte.
// A byte is the smallest addressable unit of memory.
typedef unsigned char k_byte;

// k_regn is an unsigned integer type that represents the size of GPRs on the
// target architecture. In x86_64, this is 64 bits.
// This type should be able to be freely cast to and from a pointer.
typedef uint64_t k_regn;

// k_desc is a generic descriptor.
// A descriptor, in this context, is 64 bits that contain information about
// something. A descriptor may be describing an interrupt handler, or a
// region of memory, or some other thing.
typedef uint64_t k_desc;

// PTR_TO_N converts a pointer to an unsigned integer.
#define PTR_TO_N(p) ((k_regn)p)

#endif