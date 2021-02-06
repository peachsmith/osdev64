#include "osdev64/axiom.h"
#include "osdev64/sync.h"
#include "osdev64/apic.h"
#include "osdev64/task_demo.h"


#include "klibc/stdio.h"

extern k_spinlock* g_demo_lock;
extern k_semaphore* g_demo_sem_pub;
extern k_semaphore* g_demo_sem_sub;

k_regn g_mutex_data[3];
k_regn g_sem_data[3];


//==========================================
// BEGIN mutex demo
//==========================================
void demo_mutex_task_a_action()
{
  for (int i = 0; i < 10; i++)
  {
    k_spinlock_acquire(g_demo_lock);
    k_apic_wait(120);
    fprintf(stddbg, "Mutex task A has the spinlock.\n");
    k_spinlock_release(g_demo_lock);

    k_apic_wait(150);
  }
  fprintf(stddbg, "Mutex task A has ended.\n");
}

void demo_mutex_task_b_action()
{
  for (int i = 0; i < 10; i++)
  {
    k_spinlock_acquire(g_demo_lock);
    k_apic_wait(120);

    fprintf(stddbg, "Mutex task B has the spinlock.\n");
    k_spinlock_release(g_demo_lock);

    k_apic_wait(180);
  }
  fprintf(stddbg, "Mutex task B has ended.\n");
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
    k_semaphore_wait(g_demo_sem_sub);
    fprintf(
      stddbg,
      "Semaphore task A has performed an action. "
      "sub: %lld, pub: %lld\n",
      *g_demo_sem_sub,
      *g_demo_sem_pub
    );
    k_apic_wait(120);
    k_semaphore_signal(g_demo_sem_pub);
  }
}

void demo_sem_task_b_action()
{
  for (;;)
  {
    k_semaphore_wait(g_demo_sem_sub);
    fprintf(
      stddbg,
      "Semaphore task B has performed an action. "
      "sub: %lld, pub: %lld\n",
      *g_demo_sem_sub,
      *g_demo_sem_pub
    );
    k_apic_wait(120);
    k_semaphore_signal(g_demo_sem_pub);
  }
}

void demo_sem_task_c_action()
{
  for (;;)
  {
    k_semaphore_wait(g_demo_sem_sub);
    fprintf(
      stddbg,
      "Semaphore task C has performed an action. "
      "sub: %lld, pub: %lld\n",
      *g_demo_sem_sub,
      *g_demo_sem_pub
    );
    k_apic_wait(120);
    k_semaphore_signal(g_demo_sem_pub);
  }
}
//==========================================
// BEGIN counting semaphore demo
//==========================================
