#ifndef JEP_SYSCALL_H
#define JEP_SYSCALL_H

#include "osdev64/axiom.h"

#define SYSCALL_SLEEP 1

// used for debugging
#define SYSCALL_FACE 0xFACE

k_regn* k_syscall(
  k_regn id,
  k_regn* regs,
  k_regn data1,
  k_regn data2,
  k_regn data3,
  k_regn data4
);

#endif