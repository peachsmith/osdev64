#ifndef JEP_TASK_DEMO_H
#define JEP_TASK_DEMO_H

// mutex demo tasks
void demo_mutex_task_a_action();
void demo_mutex_task_b_action();
void demo_mutex_task_c_action();

// counting semaphore demo tasks
void demo_sem_task_a_action();
void demo_sem_task_b_action();
void demo_sem_task_c_action();


/**
 * Demonstrates three tasks that attempt to acquire a lock.
 */
void mutex_demo_1();


/**
 * Demonstrates three tasks that consume messages from a queue
 * with one producer.
 */
void semaphore_demo_1();


#endif