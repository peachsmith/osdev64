#ifndef JEP_SYSCALL_H
#define JEP_SYSCALL_H

#include "osdev64/axiom.h"

#define SYSCALL_SLEEP 1

void k_syscall(
  k_regn id,
  k_regn arg1,
  k_regn arg2,
  k_regn arg3,
  k_regn arg4
);

#endif