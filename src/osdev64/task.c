#include "osdev64/task.h"
#include "osdev64/memory.h"
#include "osdev64/instructor.h"

#include "klibc/stdio.h"

#define TASK_NEW 0
#define TASK_RUNNING 1
#define TASK_STOPPED 2

// register indices
#define TASK_REG_SS 20
#define TASK_REG_RSP 19
#define TASK_REG_RFLAGS 18
#define TASK_REG_CS 17
#define TASK_REG_RIP 16
#define TASK_REG_RBP 1

// memory occupied by a 16 byte aligned register stack
#define TASK_REG_ALIGN (sizeof(uint64_t) * 22)

// The registers are in an array of uint64_t with the following structure:
// index  register
// 20     SS
// 19     RSP
// 18     RFLAGS
// 17     CS
// 16     RIP
// 15     RAX
// 14     RBX
// 13     RCX
// 12     RDX
// 11     R8
// 10     R9
// 9      R10
// 8      R11
// 7      R12
// 6      R13
// 5      R14
// 4      R15
// 3      RSI
// 2      RDI
// 1      RBP
// 0      padding


typedef struct k_task {
  uint64_t* regs; // registers
  uint64_t id;    // task ID  
  int status;     //
}k_task;

uint64_t g_task_count = 0;

k_task g_main_task;
k_task g_task_a;
k_task g_task_b;

k_task* g_current_task = &g_main_task;

unsigned char* task_a_stack;
unsigned char* task_b_stack;

uint64_t task_a_regs[21];
uint64_t task_b_regs[21];

void task_a_action()
{
  for (;;)
  {
    fprintf(stddbg, "This is task A\n");
  }
}

void task_b_action()
{
  for (;;)
  {
    fprintf(stddbg, "This is task B\n");
  }
}

void k_task_init()
{
  g_task_a.id = 2;
  g_task_a.status = TASK_RUNNING;
  task_a_stack = (unsigned char*)k_memory_alloc_pages(16);
  if (task_a_stack == NULL)
  {
    fprintf(stderr, "failed to allocate stack for task A\n");
    g_task_a.id = TASK_STOPPED;
  }
  g_task_a.regs = task_a_regs;
  g_task_a.regs[TASK_REG_RIP] = (uint64_t)task_a_action;
  g_task_a.regs[TASK_REG_RBP] = (uint64_t)task_a_stack + 0xF000;
  g_task_a.regs[TASK_REG_RSP] = (uint64_t)task_a_stack + 0xF000 - TASK_REG_ALIGN;
  g_task_a.regs[TASK_REG_RFLAGS] = 0x200;
  g_task_a.regs[TASK_REG_SS] = 0x10;
  g_task_a.regs[TASK_REG_CS] = 0x8;


  g_task_b.id = 3;
  g_task_b.status = TASK_RUNNING;
  task_b_stack = (unsigned char*)k_memory_alloc_pages(16);
  if (task_b_stack == NULL)
  {
    fprintf(stderr, "failed to allocate stack for task B\n");
    g_task_b.id = TASK_STOPPED;
  }
  g_task_b.regs = task_b_regs;
  g_task_b.regs[TASK_REG_RIP] = (uint64_t)task_b_action;
  g_task_b.regs[TASK_REG_RBP] = (uint64_t)task_b_stack + 0xF000;
  g_task_b.regs[TASK_REG_RSP] = (uint64_t)task_b_stack + 0xF000 - TASK_REG_ALIGN;
  g_task_b.regs[TASK_REG_RFLAGS] = 0x200;
  g_task_b.regs[TASK_REG_SS] = 0x10;
  g_task_b.regs[TASK_REG_CS] = 0x8;
}


// TODO: think of a better argument name than "task_stack"
uint64_t* k_task_switch(uint64_t* task_stack)
{
  uint64_t* next;

  // If the global task count is 0, then task switching hasn't
  // been initialized yet, so do that here.
  if (g_task_count == 0)
  {
    fprintf(stddbg, "initializing main task\n");
    g_main_task.id = 1;
    g_main_task.status = TASK_RUNNING;
    g_main_task.regs = task_stack;

    g_current_task = &g_main_task;

    g_task_count++;

    return g_current_task->regs;
  }

  g_current_task->regs = task_stack;

  switch (g_current_task->id)
  {
  case 1:
  {
    if (g_task_a.status == TASK_RUNNING)
    {
      fprintf(stddbg, "switching to task A\n");
      g_current_task = &g_task_a;
    }
  }
  break;

  case 2:
  {
    if (g_task_b.status == TASK_RUNNING)
    {
      fprintf(stddbg, "switching to task B\n");
      g_current_task = &g_task_b;
    }
  }
  break;

  case 3:
  {
    if (g_main_task.status == TASK_RUNNING)
    {
      fprintf(stddbg, "switching to main task\n");
      g_current_task = &g_main_task;
    }
  }
  break;

  default:
  {
    if (g_main_task.status == TASK_RUNNING)
    {
      g_current_task = &g_main_task;
    }
  }
  break;
  }

  return g_current_task->regs;
}
