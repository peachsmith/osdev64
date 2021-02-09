#ifndef JEP_TASK_H
#define JEP_TASK_H

#include "osdev64/axiom.h"


// task states
#define TASK_NEW 0
#define TASK_RUNNING 1
#define TASK_SLEEPING 2
#define TASK_STOPPED 3
#define TASK_REMOVED 4


// task structure
typedef struct k_task {
  void* mem_base;      // base address of all task memory
  k_regn* regs;        // register stack
  uint64_t id;         // task ID
  int status;          // status
  struct k_task* next; // next task in the list
  k_regn* sync_val;    // synchronization value
  k_regn sync_type;    // synchronization type
  k_regn ticks;        // timer tick count
  k_regn limit;        // timer tick limit (for sleeping)
}k_task;


/**
 * Initializes task management.
 * This function must be called before any other functions from
 * the task interface are called.
 */
void k_task_init();

/**
 * Determines which task is currently executing.
 */
k_regn* k_task_switch(k_regn*);


/**
 * Creates a new task.
 *
 * Params:
 *   void (action)() - the starting point of execution for the task
 *
 * Returns:
 *   k_task* - a pointer to a new task
 */
k_task* k_task_create(void (action)());


/**
 * Frees the memory allocated for a task.
 *
 * Params:
 *   k_task* - pointer to a task
 */
void k_task_destroy(k_task*);


/**
 * Schedules a task for execution.
 * The task should have a status of NEW.
 * Once a task has been scheduled, its memory is owned by the scheduler
 * until it is removed from the task list.
 *
 * Params:
 *   k_task* - pointer to a task to be scheduled
 */
void k_task_schedule(k_task*);

/**
 * Changes the current task's status to STOPPED and then calls k_task_switch
 * to switch to a task with a status of RUNNING.
 *
 * Params:
 *   k_regn* - a pointer to the current task's register stack
 */
k_regn* k_task_stop(k_regn*);

/**
 * Changes the current task's status to SLEEPING and then calls k_task_switch
 * to switch to a task with a status of RUNNING.
 * The first argument is a pointer to the current task's register stack,
 * the second argument is a pointer toa  synchronization value, and the
 * third argument indicates the type of synchronization value.
 * If the type is 1, then the synchronization value is a lock. If the type
 * is 2, then the synchronization value is a semaphore.
 *
 * Params:
 *   k_regn* - a pointer to the current task's register stack
 *   k_regn* - a pointer to a synchronization primitive
 *   int - the type of synchronization primitive (1 for lock, 2 for semaphore)
 */
k_regn* k_task_sleep(k_regn*, k_regn*, k_regn, k_regn);

#endif