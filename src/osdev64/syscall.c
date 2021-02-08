
#include "osdev64/syscall.h"
#include "osdev64/task.h"
#include "osdev64/instructor.h"

#include "klibc/stdio.h"


void k_syscall(
  k_regn id,
  k_regn arg1,
  k_regn arg2,
  k_regn arg3,
  k_regn arg4
)
{
  // current expected register values for syscall
  // RAX syscall ID
  // RCX syscall arg 1
  // RDX syscall arg 2
  // RSI syscall arg 3
  // RDI syscall arg 4

  if (id == SYSCALL_SLEEP)
  {
    int64_t* val = (int64_t*)arg2;

    if (arg1 == 1)
    {
      fprintf(stddbg, "[SYSCALL] SLEEP waiting for lock %lld\n", *val);
    }
    else
    {
      fprintf(stddbg, "[SYSCALL] SLEEP waiting for semaphore %lld\n", *val);
    }
  }
}
