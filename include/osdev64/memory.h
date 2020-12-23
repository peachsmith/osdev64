#ifndef JEP_MEMORY_H
#define JEP_MEMORY_H


#include <stddef.h>


/**
 * Initializes the memory manager.
 */
void k_memory_init();

/**
 * Reserves a contiguous series of pages.
 *
 * Params:
 *   size_t - the number of pages requested
 *
 * Returns:
 *   void* - a pointer to the region of memory or NULL on failure
 */
void* k_memory_alloc_pages(size_t);

/**
 * Frees a contiguous series of pages for use.
 *
 * Params:
 *   void* - a pointer to a series of pages
 */
void k_memory_free_pages(void*);

/**
 * Prints the contents of the RAM pool.
 */
void k_memory_print_pool();

/**
 * Prints the contents of the page reservation ledger.
 */
void k_memory_print_ledger();

#endif