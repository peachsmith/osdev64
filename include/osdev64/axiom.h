#ifndef JEP_AXIOM_H
#define JEP_AXIOM_H

// This file contains assumptions about the environment and target
// architecture.

// The core assumptions:

//--------------------------------
// Target Architecture Assumptions
//--------------------------------
// The target architecture is x86_64
// The target architecture has 64-bit general purpose registers

//-----------------------
// Build Tool Assumptions
//-----------------------
// The compiler toolchain is GCC built without red zone
// The compiler toolchain supports at least the C99 standard with stdint.h
// The compiler toolchain outputs 64-bit code that uses the System V ABI


//---------------------
// Firmware Assumptions
//---------------------
// The firmware will comply with the UEFI specification, which, at the time
// of writing this, can be found at https://www.uefi.org/specifications/

#endif