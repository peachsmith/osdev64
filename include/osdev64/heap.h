#ifndef JEP_HEAP_H
#define JEP_HEAP_H


// Heap Interface
//
// Functions and data types for managing dynamic memory at the byte level.
// The heap interface can be used to allocate and free individual bytes ot
// collections of bytes, as opposed to the physical memory manager, which
// allocates and free memory in pages of 4096 bytes.


#include "osdev64/axiom.h"
#include <stddef.h>


/**
 * Initializes the heap interface.
 * This must be called before any other functions in this interface.
 */
void k_heap_init();


/**
 * Allocates n bytes of memory for general use.
 *
 * Params:
 *   size_t - the number of bytes to allocate
 *
 * Returns:
 *   void* - the base address of the region of newly allocated memory
 */
void* k_heap_alloc(size_t);


/**
 * Frees a previously allocate region of memory for future use.
 *
 *
 * Params:
 *   void* - the base address of a region of allocated mmeory
 */
void k_heap_free(void*);


/**
 * Writes the state of a heap to some output stream.
 */
void k_heap_print();


#endif