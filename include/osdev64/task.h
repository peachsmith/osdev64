#ifndef JEP_TASK_H
#define JEP_TASK_H

#include <stdint.h>

typedef struct k_task {
  void* mem_base; // base address of all task memory
  uint64_t* regs; // CPU state
  uint64_t id;
  int status;
  struct k_task* next;
}k_task;

/**
 * Initializes task switching.
 *
 */
void k_task_init();


/**
 * Switches between tasks.
 *
 */
uint64_t* k_task_switch(uint64_t*);


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
 *
 * Params:
 *   k_task* - pointer to a task to be scheduled
 */
void k_task_schedule(k_task*);

/**
 * Marks a task as STOPPED.
 * Tasks with this status may be removed.
 */
void k_task_stop(k_task*);

#endif