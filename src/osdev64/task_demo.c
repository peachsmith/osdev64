#include "osdev64/axiom.h"
#include "osdev64/sync.h"
#include "osdev64/apic.h"
#include "osdev64/task.h"
#include "osdev64/task_demo.h"
#include "osdev64/syscall.h"

#include "klibc/stdio.h"

k_lock* demo_lock;
k_semaphore* demo_sem_producer;
k_semaphore* demo_sem_consumer;

k_regn g_mutex_data[3];
k_regn g_sem_data[3];


// Only one task should be able to modify this value at any given time.
static int mutex_data;


// Three contenders for a mutex lock.
// One does busy waiting, the other two sleep.
void mutex_demo_1()
{
  mutex_data = 0;

  demo_lock = k_mutex_create();
  if (demo_lock == NULL)
  {
    fprintf(stddbg, "failed to create demo spinlock\n");
    return;
  }

  k_task* a = k_task_create(demo_mutex_task_a_action);
  k_task* b = k_task_create(demo_mutex_task_b_action);
  k_task* c = k_task_create(demo_mutex_task_c_action);

  k_task_schedule(a);
  k_task_schedule(b);
  k_task_schedule(c);

  while (a != NULL || b != NULL || c != NULL)
  {
    if (a != NULL && a->status == TASK_REMOVED)
    {
      k_task_destroy(a);
      a = NULL;
    }

    if (b != NULL && b->status == TASK_REMOVED)
    {
      k_task_destroy(b);
      b = NULL;
    }

    if (c != NULL && c->status == TASK_REMOVED)
    {
      k_task_destroy(c);
      c = NULL;
    }
  }

  if (a != NULL || b != NULL || c != NULL)
  {
    fprintf(
      stddbg,
      "mutex demo 1 failed: could not dispose of all tasks\n"
    );
    return;
  }

  if (a != NULL || b != NULL || c != NULL)
  {
    fprintf(
      stddbg,
      "mutex demo 1 failed: expected: 3, actual: %d\n",
      mutex_data
    );
    return;
  }

  fprintf(
    stddbg,
    "mutex demo 1 passed\n",
    mutex_data
  );
}



void semaphore_demo_1()
{

}



//==========================================
// BEGIN mutex demo
//==========================================
void demo_mutex_task_a_action()
{
  for (int i = 0; i < 1; i++)
  {
    k_mutex_acquire(demo_lock, SYNC_SLEEP);
    // k_syscall_sleep(120);

    mutex_data++;
    fprintf(stddbg, "Mutex task A has the lock.\n");

    k_mutex_release(demo_lock);
    // k_syscall_sleep(150);
  }
}

void demo_mutex_task_b_action()
{
  for (int i = 0; i < 1; i++)
  {
    k_mutex_acquire(demo_lock, SYNC_SPIN);
    k_syscall_sleep(120);

    mutex_data++;
    fprintf(stddbg, "Mutex task B has the lock.\n");

    k_mutex_release(demo_lock);
    // k_syscall_sleep(180);
  }
}

void demo_mutex_task_c_action()
{
  for (int i = 0; i < 1; i++)
  {
    k_mutex_acquire(demo_lock, SYNC_SLEEP);
    // k_syscall_sleep(110);

    mutex_data++;
    fprintf(stddbg, "Mutex task C has the lock.\n");

    k_mutex_release(demo_lock);
    // k_syscall_sleep(160);
  }
}
//==========================================
// END mutex demo
//==========================================




//==========================================
// BEGIN counting semaphore demo
//==========================================

void demo_sem_task_a_action()
{
  for (;;)
  {
    k_semaphore_wait(demo_sem_consumer, SYNC_SLEEP);
    fprintf(
      stddbg,
      "Semaphore task A has performed an action. "
      "sub: %lld, pub: %lld\n",
      *demo_sem_consumer,
      *demo_sem_producer
    );
    k_syscall_sleep(120);
    k_semaphore_signal(demo_sem_producer);
  }
}

void demo_sem_task_b_action()
{
  for (;;)
  {
    k_semaphore_wait(demo_sem_consumer, SYNC_SPIN);
    fprintf(
      stddbg,
      "Semaphore task B has performed an action. "
      "sub: %lld, pub: %lld\n",
      *demo_sem_consumer,
      *demo_sem_producer
    );
    k_syscall_sleep(120);
    k_semaphore_signal(demo_sem_producer);
  }
}

void demo_sem_task_c_action()
{
  for (;;)
  {
    k_semaphore_wait(demo_sem_consumer, SYNC_SPIN);
    fprintf(
      stddbg,
      "Semaphore task C has performed an action. "
      "sub: %lld, pub: %lld\n",
      *demo_sem_consumer,
      *demo_sem_producer
    );
    k_syscall_sleep(120);
    k_semaphore_signal(demo_sem_producer);
  }
}
//==========================================
// BEGIN counting semaphore demo
//==========================================
