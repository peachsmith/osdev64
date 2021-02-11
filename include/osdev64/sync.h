#ifndef JEP_SYNC_H
#define JEP_SYNC_H

// Synchronization Interface
//
// This interface contains function and data types for synchronizing access
// to resources across multiple tasks. Mutual exclusion is achieved through
// the use of locks, and producer/consumer synchronization is achieved
// through semaphores.

#include "osdev64/axiom.h"


// Waiting types
// When a task is waiting on a synchronization value, it may sleep or spin.
// Sleeping, is when a task is not given any execution time by the scheduler.
// The scheduler will check the synchronization value to determine if the
// task can resume execution.
// Spinning is when the task enters a loop and repeatedly checks the
// synchronization value to see if it has become available. Spinning is
// sometimes referred to as "busy waiting"
#define SYNC_SLEEP 0
#define SYNC_SPIN 1


/**
 * A lock is a binary value that a task sets to gain access to a resource.
 * This is used to implement mutual exclusion. When a given task acquires
 * the lock, no other tasks should be able to acquire the lock until the
 * lock is released. Any task that attempts to acquire a lock that is
 * already acquired must wait until the lock becomes available.
 *
 * A task may acquire a lock by calling k_mutex_acquire, at which point
 * it now owns the lock for the duration of the critical section.
 * Once the task has completed its work in the critical section, it
 * must release the lock by calling k_mutex_release, at which point
 * other tasks may acquire the lock.
 */
typedef k_regn k_lock;

/**
 * A sempahore is a value that is decremented and incremented to control
 * access to resources by multiple tasks. A semaphore with a starting value
 * of n can allow up to n tasks to access a resource simultaneously. Access
 * is gained by decrementing the semaphore, and availability is broadcast
 * by incrementing the semaphore.
 *
 * A task attempts to gain access to a resource through a semaphore by
 * calling k_semaphore_wait. Once it has successfully decremented the value,
 * it can proceed. A task can notify other tasks waiting on a semaphore by
 * calling k_semaphore_signal. This function increments the semaphore.
 */
typedef k_regn k_semaphore;

/**
 * Initializes the synchronization interface.
 * This must be called before any other functions in this interface.
 */
void k_sync_init();


/**
 * Creates a new mutex lock.
 *
 * Returns:
 *   k_lock* - a pointer to a new lock or NULL on failure
 */
k_lock* k_mutex_create();


/**
 * Frees the memory allocated for a mutex lock.
 *
 * Params:
 *   k_lock* - a pointer to the lock to destroy
 */
void k_mutex_destroy(k_lock*);

/**
 * Attempts to acquire a lock to ensure mutual exclusion.
 * The task that calls this function will wait until the lock becomes
 * available. The first argument is a pointer to the lock, and the
 * second argument is the busy flag.
 * If the busy flag is 1, then this function will loop until the lock
 * becomes available. If the busy flag is 0, then the current task
 * will be put to sleep until the lock becomes available.
 *
 * Params:
 *   k_lock* - a pointer to the spinlock to acquire
 *   int - busy flag (1 for busy wait, 0 for sleeping wait)
 */
void k_mutex_acquire(k_lock*, int);

/**
 * Releases a lock.
 *
 * Params:
 *   k_lock* - a pointer to the lock to release
 */
void k_mutex_release(k_lock*);


/**
 * Creates a new counting sempahore.
 *
 * Params:
 *   int64_t - the starting value of the semaphore
 *
 * Returns:
 *   k_semaphore* - a pointer to a new semaphore or NULL on failure
 */
k_semaphore* k_semaphore_create(int64_t n);


/**
 * Frees the memory allocated for a semaphore.
 *
 * Returns:
 *   k_semaphore* - a pointer to the sempahore to destroy
 */
void k_semaphore_destroy(k_semaphore*);


/**
 * Attempts to decrement a semaphore. The first argument is a pointer to the
 * semaphore, and the second argument is the busy flag.
 * If the busy flag is 1, then this function will loop until the semaphore has
 * a value of > 0. If the busy flag is 0, then the current task will be put
 * to sleep until the semaphore has a value of > 0
 *
 * Params:
 *   k_semaphore* - a pointer to the semaphore to be decremented
 */
void k_semaphore_wait(k_semaphore*, int);


/**
 * Increments the value of a sempahore by 1 to allow an additional task
 * to access a resource.
 *
 * Params:
 *   k_semaphore* - a pointer to the semaphore to be incremented
 */
void k_semaphore_signal(k_semaphore*);

#endif