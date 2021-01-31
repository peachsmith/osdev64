#ifndef JEP_SYNCHRONIZATION_H
#define JEP_SYNCHRONIZATION_H

// Synchronization Interface

#include "osdev64/axiom.h"

/**
 * A spinlock is a lock in which the task attempting to obtain the lock
 * waits for the lock to become available.
 */
typedef k_regn k_spinlock;

void k_sync_init();

/**
 * Creates a new spinlock.
 *
 * Returns:
 *   k_spinlock* - a pointer to a new spinlock or NULL on failure
 */
k_spinlock* k_spinlock_create();


/**
 * Frees the memory allocated for a spinlock.
 *
 * Returns:
 *   k_spinlock* - a pointer to the spinlock to destroy
 */
void k_spinlock_destroy(k_spinlock*);

/**
 * Attempts to acquire a spinlock.
 * The task that calls this function will wait until the lock becomes
 * available.
 *
 * Params:
 *   k_spinlock* - a pointer to the spinlock to acquire
 */
void k_spinlock_acquire(k_spinlock*);

/**
 * Releases a spinlock.
 *
 * Params:
 *   k_spinlock* - a pointer to the spinlock to release
 */
void k_spinlock_release(k_spinlock*);

#endif