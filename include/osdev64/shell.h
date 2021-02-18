#ifndef JEP_SHELL_H
#define JEP_SHELL_H

// Kernel Shell Interface
//
// Function and data types for a kernel-mode shell.

#include "osdev64/axiom.h"


void k_shell_init();

void* k_shell_get_stdout();

#endif