#include "osdev64/task.h"
#include "osdev64/memory.h"
#include "osdev64/instructor.h"
#include "osdev64/apic.h"

#include "klibc/stdio.h"

// register stack indices
#define TASK_REG_SS 20
#define TASK_REG_RSP 19
#define TASK_REG_RFLAGS 18
#define TASK_REG_CS 17
#define TASK_REG_RIP 16
#define TASK_REG_RBP 1

// memory occupied by a 16 byte aligned register stack
// This represents a number of bytes of memory that is at least
// large enough to hold the register stack of a task as well as
// a function to be called once the task returns from its action.
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
// | //// unknown //// | <- don't know, don't care, don't touch
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



// number of tasks that have been created
uint64_t g_task_count = 1;

// the initial memory pointed to by the current task pointer
k_task g_primer_task;

// currently task
k_task* g_current_task = &g_primer_task;

// global task list
k_task* g_task_list = NULL;


/**
 * Removes a task from the global task list.
 * This function does not free the memory used by a task.
 *
 * Params:
 *   k_task* - a pointer to the task to be removed
 */
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
    target->status = TASK_REMOVED;
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
  fprintf(stddbg, "a task has ended\n");

  // Mark the task as STOPPED so it will be removed.
  g_current_task->status = TASK_STOPPED;

  // Hang and wait for task disposal.
  for (;;);
}

k_task* k_task_create(void (action)())
{
  void* task_mem; // task memory
  uint64_t rsp;   // stack pointer
  uint64_t rbp;   // base pointer

  // Allocate 16 Kib of stack space, and 4 Kib for the task state.
  // The task memory will have the following layout:
  // +--------------------+
  // |      4 Kib         |
  // |   task state and   |
  // |  register values   |
  // |--------------------|
  // |                    |
  // |       16 Kib       |
  // |     stack space    |
  // |                    |
  // +--------------------+
  task_mem = k_memory_alloc_pages(5);
  if (task_mem == NULL)
  {
    return NULL;
  }

  // Calculate the initial stack and base pointer.
  // The stack pointer should be 16 byte aligned, and we should
  // reserve space on the initial stack for the ISR stack, the
  // register values, and the k_task_end function.
  rsp = (uint64_t)task_mem + 0x4000 - TASK_STACK_ALIGN;
  rbp = (uint64_t)task_mem + 0x4000;

  // Put the address of the k_task_end function on this task's stack.
  *(uint64_t*)(rsp) = (uint64_t)k_task_end;

  // The memory that will hold the task state will start
  // at at an offset of 16 bytes from the start of the fifth page.
  k_task* task = (k_task*)((uint64_t)task_mem + 0x4010);

  // The register stack memory will start at at an offset of 96 bytes
  // from the start of the fifth page.
  // This leaves a difference of 80 bytes between the start of task state
  // memory and the start of register stack memory.
  task->regs = (uint64_t*)((uint64_t)task_mem + 0x4060);


  // Build an ISR stack whose values will be popped
  // off the stack by the IRET instruction.
  task->regs[TASK_REG_SS] = 0x10;              // data segment
  task->regs[TASK_REG_RSP] = rsp;              // stack pointer
  task->regs[TASK_REG_RFLAGS] = 0x200;         // bit 9 (interrupt flag)
  task->regs[TASK_REG_CS] = 0x8;               // code segment.
  task->regs[TASK_REG_RIP] = (uint64_t)action; // begin execution here


  // Set the initial base pointer.
  task->regs[TASK_REG_RBP] = rbp;

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
