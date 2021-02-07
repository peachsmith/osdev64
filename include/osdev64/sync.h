#ifndef JEP_SYNCHRONIZATION_H
#define JEP_SYNCHRONIZATION_H

// Synchronization Interface

#include "osdev64/axiom.h"


/**
 * A lock in which the task attempting to obtain the lock
 * waits for the lock to become available. This is used to implement
 * mutual exclusion. Once a task has acquired the lock, no other tasks
 * should be able to acquire the lock until it is released.
 *
 * A task utilizes a spinlock through the k_spinlock_acquire and
 * k_spinlock_release functions. A task acquires the lock by calling
 * k_spinlock_acquire, then it performs the desired operations
 * on the protected resource, then it releases the lock by calling
 * k_spinlock_release.
 */
typedef k_regn k_lock;

/**
 * A sempahore allows one or more tasks to notify other tasks when
 * a resource is available. For a given semaphore with a value of
 * n, up to n tasks can access the synchronized resource at any given time.
 *
 * Tasks utilize a sempahore with the k_sempahore_wait and
 * k_sempahore_signal functions. A task that attempts to access a protected
 * resource calls k_sempahore_wait to make sure that the resource is available.
 * Once the resource is available, the task can then access it.
 * When a task wants to notify other tasks that a resource has become available,
 * it calls k_sempahore_signal.
 */
typedef k_regn k_semaphore;

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
 * Returns:
 *   k_lock* - a pointer to the lock to destroy
 */
void k_mutex_destroy(k_lock*);

/**
 * Attempts to acquire a lock to ensure mutual exclusion.
 * The task that calls this function will wait until the lock becomes
 * available. The first argument is a pointer to the lock, and the
 * second argument is the busy flag.
 * If the busy flag is 1, then this function wil loop until the lock
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
 * a value of >= 0. If the busy flag is 0, then the current task will be put
 * to sleep until the semaphore has a value of >= 0
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