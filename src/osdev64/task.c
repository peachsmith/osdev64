#include "osdev64/task.h"
#include "osdev64/memory.h"
#include "osdev64/instructor.h"

#include "klibc/stdio.h"


// TODO:
// implement task creation
// implement task destruction
// implement more task switching logic
// refactor and define the task life cycle


// task states
#define TASK_NEW 0
#define TASK_RUNNING 1
#define TASK_STOPPED 2

// register stack indices
#define TASK_REG_SS 20
#define TASK_REG_RSP 19
#define TASK_REG_RFLAGS 18
#define TASK_REG_CS 17
#define TASK_REG_RIP 16
#define TASK_REG_RBP 1

// memory occupied by a 16 byte aligned register stack
#define TASK_STACK_ALIGN (sizeof(uint64_t) * 22)

// number of values in the register stack including padding
#define TASK_REG_COUNT 21

// The register stack is an array of 64-bit values passed by the ISR.
// Upon entering the ISR, the stack is 16 byte aligned and contains
// the values of SS, RSP, RFLAGS, CS, and RIP from before the interrupt
// was raised.
//
// Register Stack Structure
// +-------------------+
// | ///////////////// |
// | //// unknown //// |<- don't know, don't care, don't touch
// | ///////////////// |
// |-------------------|
// | pushed by the CPU |
// |                   |
// | 20     SS         |
// | 19     RSP        | <- RSP from before entering the ISR
// | 18     RFLAGS     |
// | 17     CS         |
// | 16     RIP        | <- where execution will resume
// |-------------------|
// | pushed by the ISR |
// |                   |
// | 15     RAX        |
// | 14     RBX        |
// | 13     RCX        |
// | 12     RDX        |
// | 11     R8         |
// | 10     R9         |
// | 9      R10        |
// | 8      R11        |
// | 7      R12        |
// | 6      R13        |
// | 5      R14        |
// | 4      R15        |
// | 3      RSI        |
// | 2      RDI        |
// | 1      RBP        |
// | 0      padding    | <- address that we receive from ISR
// +-------------------+


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

// Populates a task with everything it needs to succeed in life.
static void init_task(
  k_task* task,
  uint64_t* regs,
  void (action)(),
  uint64_t id
)
{
  task->id = id;
  task->status = TASK_RUNNING;

  // Allocate a stack for the task.
  // We want 64 KiB, so we allocate 17 pages so that the initial
  // stack base can be a multiple of 4096.
  void* stack = k_memory_alloc_pages(17);
  if (stack == NULL)
  {
    task->status = TASK_STOPPED;
    return;
  }

  task->regs = regs;
  task->regs[TASK_REG_RIP] = (uint64_t)action;

  // The stack is 64 KiB, so the base pointer starts at an offset
  // of 0x10000 from the pointer that was allocated earlier.
  task->regs[TASK_REG_RBP] = (uint64_t)stack + 0x10000;

  // The initial stack pointer should be 16 byte aligned and allow
  // for the contents of an interrupt handler stack.
  task->regs[TASK_REG_RSP] = (uint64_t)stack + 0x10000 - TASK_STACK_ALIGN;

  // Set the default RFLAGS value.
  // The only bit that is set is bit 9, the interrupt flag (IF).
  task->regs[TASK_REG_RFLAGS] = 0x200;

  // Set the offset of kernel mode data segment.
  task->regs[TASK_REG_SS] = 0x10;

  // Set the offset of kernel mode code segment.
  task->regs[TASK_REG_CS] = 0x8;
}


void k_task_init()
{
  init_task(&g_task_a, task_a_regs, task_a_action, 2);

  init_task(&g_task_b, task_b_regs, task_b_action, 3);
}


uint64_t* k_task_switch(uint64_t* reg_stack)
{
  // If we haven't saved the state of the main kernel task,
  // do that here and return.
  if (g_task_count == 0)
  {
    g_main_task.id = 1;
    g_main_task.status = TASK_RUNNING;
    g_main_task.regs = reg_stack;

    g_current_task = &g_main_task;

    g_task_count++;

    return g_current_task->regs;
  }

  // Save the register stack of the current task.
  g_current_task->regs = reg_stack;

  // Select the next task.
  // Currently, we just switch between three known tasks:
  // task A, task B, and the main kernel task.
  switch (g_current_task->id)
  {
  case 1:
  {
    if (g_task_a.status == TASK_RUNNING)
    {
      g_current_task = &g_task_a;
    }
  }
  break;

  case 2:
  {
    if (g_task_b.status == TASK_RUNNING)
    {
      g_current_task = &g_task_b;
    }
  }
  break;

  case 3:
  {
    if (g_main_task.status == TASK_RUNNING)
    {
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

  // Return the register stack of the next task.
  return g_current_task->regs;
}
