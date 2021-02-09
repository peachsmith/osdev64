
#include "osdev64/syscall.h"
#include "osdev64/task.h"
#include "osdev64/instructor.h"

#include "klibc/stdio.h"


k_regn* k_syscall(
  k_regn id,
  k_regn* regs,
  k_regn data1,
  k_regn data2,
  k_regn data3,
  k_regn data4
)
{
  // syscall arguments
  //
  // ARG 1:  syscall ID
  // ARG 2:  task register stack
  // ARG 3:  syscall arg 1
  // ARG 4:  syscall arg 2
  // ARG 5:  syscall arg 3
  // ARG 6:  syscall arg 4

  switch (id)
  {

  case SYSCALL_SLEEP:
  {
    // data1 is the synchronization type
    // data2 is the synchronization value

    return k_task_sleep(regs, (k_regn*)data2, data1);
  }

  case SYSCALL_STOP:
  {
    return k_task_stop(regs);
  }

  case SYSCALL_FACE:
    fprintf(stddbg, "This is the FACE syscall. Data: %llX\n", data1);
    return regs;

  default:
    return regs;
  }
}
