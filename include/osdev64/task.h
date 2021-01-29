#ifndef JEP_TASK_H
#define JEP_TASK_H

#include <stdint.h>

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

#endif