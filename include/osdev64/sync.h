#ifndef JEP_SYNCHRONIZATION_H
#define JEP_SYNCHRONIZATION_H

// Synchronization Interface

#include "osdev64/axiom.h"


// Synchronization notes:
//
// Mutex:
// "mutex" is an abbreviation of "mutual exclusion". It is the concept
// of only one task being allowed access to a resource at any given time.
// 
// Sempahore:
// A sempahore is a value that is decremented to gain access to a resource.
// A semaphore with a starting value of n can theoretically allow up to n
// tasks to access some resoure simultaneously.
//
// Spinlock:
// A spinlock attempts to set a value and enters into a loop until that value
// is successfully set. This loop is sometimes called "busy waiting" because
// it uses CPU clock cycles. Due to the loop taking up CPU time, it's
// generally regarded as inefficient in a single core environment.
//
// Sleep:
// In the context of task synchronization, to "sleep" is to wait on some
// condition without expending CPU clock cycles. This is an alternative
// to the busy waiting of a spinlock. When the scheduler is selecting the
// next task to run, it will skip over a task that is sleeping until
// the resource that the task is waiting to access becomes available.
//
//
// Type Definitions:
//
//   mutex_lock - an implementation of a lock that uses either spinlocks
//                or sleeping to acquire the lock.
//
//   sempahore - an implementation of a counting semaphore that uses either
//               spinlocks or sleeping to ensure the atomicity of its
//               increment and decrement operations.
//
//   spinlock - an implementation of a spinlock that attempts to set a
//              value and loops infinitely until the value is successfully
//              set.
//



/**
 * A spinlock is a lock in which the task attempting to obtain the lock
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
typedef k_regn k_spinlock;

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

/**
 * Attempts to acquire a lock.
 * The task that calls this function will sleep until the lock becomes
 * available.
 *
 * Params:
 *   k_spinlock* - a pointer to the spinlock to acquire
 */
void k_lock_acquire(k_spinlock*);




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
 * Frees the memory allocated for a spinlock.
 *
 * Returns:
 *   k_spinlock* - a pointer to the sempahore to destroy
 */
void k_semaphore_destroy(k_semaphore*);

/**
 * Decrements a semaphore by 1 to attempt to access a resource.
 *
 * Params:
 *   k_semaphore* - a pointer to the semaphore to be decremented
 */
void k_semaphore_wait(k_semaphore*);

/**
 * Increments the value of a sempahore by 1 to allow an additional task
 * to access a resource.
 *
 * Params:
 *   k_semaphore* - a pointer to the semaphore to be incremented
 */
void k_semaphore_signal(k_semaphore*);

#endif