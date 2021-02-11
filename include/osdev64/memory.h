#ifndef JEP_MEMORY_H
#define JEP_MEMORY_H


// Memory Interface.
// Functions and data types for managing physical RAM.
//
// The function k_memory_init must be called before any other functions in
// this interface are called.
// During initialization, the memory map is obtained from the firmware, and
// regions of usable memory are collected from it into a structure called the
// RAM pool. Each region of memory is expected to be aligned on a 4KiB
// boundary.
//
// Once the RAM pool is created, a structure called the RAM ledger is
// created to keep record of allocations of regions of memory.


#include <stddef.h>


/**
 * Initializes the memory manager.
 * This must be called before any other functions in this interface.
 * This function also calls the UEFI function ExitBootServices, so once
 * this function is called, UEFI boot service will no longer be available.
 *
 * The following actions are performed:
 *   1. obtain the memory map from the firmware
 *   2. create the RAM pool
 *   3. exit UEFI boot services
 *   3. create the RAM ledger
 */
void k_memory_init();


/**
 * Reserves a contiguous series of pages.
 * A physical page is 4096 bytes (4 KiB). The argument to this function
 * specifies a number of pages to be reserved for use by the system.
 *
 * This function searches the RAM pool for a region that has not been
 * reserved in the RAM ledger. If it finds an available region, it creates
 * a new entry in the RAM ledger to mark that region as reserved.
 *
 * Upon successfully allocating the requested number of pages, a pointer
 * to the base of the region of memory is returned. If the memory could not
 * be allocated, then NULL is returned.
 *
 * Params:
 *   size_t - the number of pages requested
 *
 * Returns:
 *   void* - a pointer to the region of memory or NULL on failure
 */
void* k_memory_alloc_pages(size_t);


/**
 * Frees a contiguous series of pages.
 * This function looks for an entry in the RAM ledger that starts at the
 * specified address and marks it as available.
 *
 * If no entry is found in the RAM ledger with a starting address that
 * matches the argument, then this function does nothing.
 *
 * An entry in the RAM ledger that is marked as available will not
 * necessarily be cleared of its contents immediately, but it may be
 * overwritten with new values during a later allocation.
 *
 * Params:
 *   void* - a pointer to a series of pages
 */
void k_memory_free_pages(void*);


/**
 * This function writes the contents of the RAM pool to some output stream.
 * It is intended to be used for debugging. The actual output destination
 * is undefined.
 */
void k_memory_print_pool();


/**
 * This function writes the contents of the RAM ledger to some output stream.
 * It is intended to be used for debugging. The actual output destination
 * is undefined.
 */
void k_memory_print_ledger();


#endif