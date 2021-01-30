#include "osdev64/task.h"
#include "osdev64/memory.h"
#include "osdev64/instructor.h"
#include "osdev64/apic.h"

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
#define TASK_STACK_ALIGN (sizeof(uint64_t) * 24)

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




uint64_t g_task_count = 1;

k_task g_main_task;
// k_task g_task_a;
// k_task g_task_b;

k_task* g_current_task = &g_main_task;

// global task list
k_task* g_task_list = NULL;

// uint64_t task_a_regs[21];
// uint64_t task_b_regs[21];

// void task_a_action()
// {
//   for (;;)
//   {
//     fprintf(stddbg, "This is task A\n");
//   }
// }

// void task_b_action()
// {
//   for (;;)
//   {
//     fprintf(stddbg, "This is task B\n");
//   }
// }

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


static void remove_task(k_task* target)
{
  k_task* node = g_task_list;
  k_task* first = node;
  k_task* prev;

  while (node != target)
  {
    prev = node;
    node = node->next;

    // If we made it back to the first task,
    // then we couldn't find the task to remove.
    if (node == first)
    {
      return;
    }
  }

  // Remove the task from the list.
  prev->next = node->next;

  // If we only have one task left, set its next pointer to NULL,
  // so it doesn't point to itself.
  if (prev == prev->next)
  {
    prev->next = NULL;
  }
}


void k_task_init()
{
  // init_task(&g_task_a, task_a_regs, task_a_action, 2);

  // init_task(&g_task_b, task_b_regs, task_b_action, 3);
}





uint64_t* k_task_switch(uint64_t* reg_stack)
{
  // If there are no tasks, then proceed with the current task.
  if (g_task_list == NULL)
  {
    return reg_stack;
  }

  // Save the register stack of the current task.
  g_current_task->regs = reg_stack;

  // Remove any stopped tasks from the list.
  while (g_current_task->next != NULL
    && g_current_task->next->status == TASK_STOPPED)
  {
    k_task* target = g_current_task->next;
    remove_task(target);
    k_task_destroy(target);
  }

  // Select the next task.
  // Currently, we just do round robin, which is really inefficient.
  if (g_current_task->next != NULL)
  {
    g_current_task = g_current_task->next;
  }

  // Return the register stack of the next task.
  return g_current_task->regs;
}

/**
 * This function waits for the current task to be removed.
 * It's address is placed on the top of the initial stack of a task
 * so that it's called when the task returns from its action.
 */
void k_task_end()
{
  int count_down = 5;

  fprintf(stddbg, "a task has ended\n");
  for (;;)
  {
    k_apic_wait(240);
    fprintf(stddbg, "[TASK] ended task ID: %llu\n", g_current_task->id);

    if (count_down > 0)
    {
      count_down--;

      if (count_down == 0)
      {
        // Mark the task as STOPPED so it will be removed.
        g_current_task->status = TASK_STOPPED;
      }
    }
  }
}

k_task* k_task_create(void (action)())
{
  // Allocate 16 Kib of stack space, and 4 Kib for the task state.
  // The task memory will have the following layout:
  // +--------------------+
  // |      16 Kib        |
  // |    stack space     |
  // |                    |
  // |--------------------|
  // |       4 Kib        |
  // |   task state and   |
  // |   register stack   |
  // +--------------------+
  void* task_mem = k_memory_alloc_pages(5);
  if (task_mem == NULL)
  {
    return NULL;
  }

  // The memory that will hold the task state will start
  // at at an offset of 16 bytes from the start of the fifth page.
  k_task* task = (k_task*)((uint64_t)task_mem + 0x4010);

  // The register stack memory will start at at an offset of 96 bytes
  // from the start of the fifth page.
  // This leaves a difference of 80 bytes between the start of task state
  // memory and the start of register stack memory.
  task->regs = (uint64_t*)((uint64_t)task_mem + 0x4060);

  // Assign a function where execution will begin.
  task->regs[TASK_REG_RIP] = (uint64_t)action;

  // The stack is 16 KiB, so the base pointer starts at an offset
  // of 0x4000 from the pointer that was allocated earlier.
  task->regs[TASK_REG_RBP] = (uint64_t)task_mem + 0x4000;

  // The initial stack pointer should be 16 byte aligned and allow
  // for the contents of an interrupt handler stack.
  task->regs[TASK_REG_RSP] = (uint64_t)task_mem + 0x4000 - TASK_STACK_ALIGN;

  // Put the address of the k_task_end function on this task's stack.
  *((uint64_t*)(task->regs[TASK_REG_RSP])) = (uint64_t)k_task_end;

  // Set the default RFLAGS value.
  // The only bit that is set is bit 9, the interrupt flag (IF).
  task->regs[TASK_REG_RFLAGS] = 0x200;

  // Set the offset of kernel mode data segment.
  task->regs[TASK_REG_SS] = 0x10;

  // Set the offset of kernel mode code segment.
  task->regs[TASK_REG_CS] = 0x8;

  // All tasks are created with a status of NEW.
  task->status = TASK_NEW;

  // For now, the ID will just be the global task count incremented by 1.
  task->id = ++g_task_count;

  // Save the base address of task memory so it can be freed later.
  task->mem_base = task_mem;

  task->next = NULL;

  return task;
}

void k_task_destroy(k_task* t)
{
  // The memory pointed to by a task's mem_base field includes the
  // task itself, so we can just free that pointer and be done with it.
  k_memory_free_pages(t->mem_base);
}

void k_task_schedule(k_task* t)
{
  // If there are no tasks in the global list yet,
  // then this task is the first.
  if (g_task_list == NULL)
  {
    g_task_list = t;
    g_current_task = t;
    return;
  }

  k_task* node = g_task_list;

  // If there is only one task in the list,
  // create a circular linked list where the
  // two entries point to each other.
  if (node->next == NULL)
  {
    node->next = t;
    t->next = node;
    return;
  }

  // Insert the new task in between the first and second tasks.
  t->next = node->next;
  node->next = t;

  // Mark the task as RUNNING.
  t->status = TASK_RUNNING;
}

void k_task_stop(k_task* t)
{
  t->status = TASK_STOPPED;
}