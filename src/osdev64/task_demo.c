#include "osdev64/axiom.h"
#include "osdev64/sync.h"
#include "osdev64/apic.h"
#include "osdev64/task_demo.h"
#include "osdev64/syscall.h"

#include "klibc/stdio.h"

extern k_lock* g_demo_lock;
extern k_semaphore* g_demo_sem_pub;
extern k_semaphore* g_demo_sem_sub;

k_regn g_mutex_data[3];
k_regn g_sem_data[3];


//==========================================
// BEGIN mutex demo
//==========================================
void demo_mutex_task_a_action()
{
  for (int i = 0; i < 1; i++)
  {
    k_mutex_acquire(g_demo_lock, SYNC_SLEEP);
    k_syscall_sleep(120);

    fprintf(stddbg, "Mutex task A has the lock.\n");
    k_mutex_release(g_demo_lock);

    k_syscall_sleep(150);
  }
  // fprintf(stddbg, "Mutex task A has ended.\n");
}

void demo_mutex_task_b_action()
{
  for (int i = 0; i < 1; i++)
  {
    k_mutex_acquire(g_demo_lock, SYNC_SPIN);
    k_syscall_sleep(120);

    fprintf(stddbg, "Mutex task B has the lock.\n");
    k_mutex_release(g_demo_lock);

    k_syscall_sleep(180);
  }
  // fprintf(stddbg, "Mutex task B has ended.\n");
}

void demo_mutex_task_c_action()
{
  for (int i = 0; i < 1; i++)
  {
    k_mutex_acquire(g_demo_lock, SYNC_SLEEP);
    k_syscall_sleep(110);

    fprintf(stddbg, "Mutex task C has the lock.\n");
    k_mutex_release(g_demo_lock);

    k_syscall_sleep(160);
  }
  // fprintf(stddbg, "Mutex task C has ended.\n");
}
//==========================================
// END mutex demo
//==========================================




//==========================================
// BEGIN counting semaphore demo
//==========================================

void k_debug_spin(int64_t s)
{
  fprintf(stddbg, "%lld\n", s);
}

void demo_sem_task_a_action()
{
  for (;;)
  {
    k_semaphore_wait(g_demo_sem_sub, SYNC_SLEEP);
    fprintf(
      stddbg,
      "Semaphore task A has performed an action. "
      "sub: %lld, pub: %lld\n",
      *g_demo_sem_sub,
      *g_demo_sem_pub
    );
    k_syscall_sleep(120);
    k_semaphore_signal(g_demo_sem_pub);
  }
}

void demo_sem_task_b_action()
{
  for (;;)
  {
    k_semaphore_wait(g_demo_sem_sub, SYNC_SPIN);
    // fprintf(stddbg, "[TASK] B decremented sub former: %lld, current: %lld\n", sub_res, *g_demo_sem_sub);
    fprintf(
      stddbg,
      "Semaphore task B has performed an action. "
      "sub: %lld, pub: %lld\n",
      *g_demo_sem_sub,
      *g_demo_sem_pub
    );
    k_syscall_sleep(120);
    k_semaphore_signal(g_demo_sem_pub);
    // fprintf(stddbg, "[TASK] B incremented pub former: %lld, current: %lld\n", pub_res, *g_demo_sem_pub);
  }
}

void demo_sem_task_c_action()
{
  for (;;)
  {
    k_semaphore_wait(g_demo_sem_sub, SYNC_SPIN);
    // fprintf(stddbg, "[TASK] C decremented sub former: %lld, current: %lld\n", sub_res, *g_demo_sem_sub);
    fprintf(
      stddbg,
      "Semaphore task C has performed an action. "
      "sub: %lld, pub: %lld\n",
      *g_demo_sem_sub,
      *g_demo_sem_pub
    );
    k_syscall_sleep(120);
    k_semaphore_signal(g_demo_sem_pub);
    // fprintf(stddbg, "[TASK] C incremented pub former: %lld, current: %lld\n", pub_res, *g_demo_sem_pub);
  }
}
//==========================================
// BEGIN counting semaphore demo
//==========================================
