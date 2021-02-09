
#include "osdev64/syscall.h"
#include "osdev64/task.h"
#include "osdev64/instructor.h"

#include "klibc/stdio.h"


k_regn k_syscall(
  k_regn id,
  k_regn data1,
  k_regn data2,
  k_regn data3,
  k_regn data4
)
{
  switch (id)
  {

  case SYSCALL_SLEEP_SYNC:
  {
    // data1 is the register stack
    // data2 is the synchronization type
    // data3 is the synchronization value
    k_regn* next = k_task_sleep((k_regn*)data1, (k_regn*)data3, data2, 0);

    return PTR_TO_N(next);
  }

  case SYSCALL_SLEEP_TICK:
  {
    // data1 is the register stack
    // data2 is the tick limit
    k_regn* next = k_task_sleep((k_regn*)data1, NULL, 3, data2);

    return PTR_TO_N(next);
  }

  case SYSCALL_STOP:
  {
    k_regn* next = k_task_stop((k_regn*)data1);
    return PTR_TO_N(next);
  }

  case SYSCALL_FACE:
    fprintf(stddbg, "This is the FACE syscall. Data: %llX\n", data1);
    return PTR_TO_N(NULL);

  default:
    return PTR_TO_N(NULL);
  }
}
