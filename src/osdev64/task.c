#include "osdev64/task.h"
#include "osdev64/memory.h"
#include "osdev64/heap.h"
#include "osdev64/interrupts.h"
#include "osdev64/instructor.h"
#include "osdev64/apic.h"
#include "osdev64/syscall.h"
#include "osdev64/file.h"

#include "klibc/stdio.h"

// register stack indices
#define TASK_REG_SS 20
#define TASK_REG_RSP 19
#define TASK_REG_RFLAGS 18
#define TASK_REG_CS 17
#define TASK_REG_RIP 16
#define TASK_REG_RBP 1

#define TASK_SYNC_LOCK 1
#define TASK_SYNC_SEMAPHORE 2
#define TASK_SYNC_TICK 3

// Memory for the initial contents of a task's stack.
// It must be large enough to hold a task's entire register stack,
// since the registers stack is popped upon returning to the IRQ that
// called k_task_switch.
// It must also have space for the k_task_end function.
#define TASK_STACK_SPACE (sizeof(uint64_t) * 28)


// number of values in the register stack including padding
#define TASK_REG_COUNT 21

// The register stack is an array of 64-bit values passed by the ISR.
// Upon entering the ISR, the stack is 16 byte aligned and contains
// the values of SS, RSP, RFLAGS, CS, and RIP from before the interrupt
// was raised.
//
// Register Stack Structure
// +-------------------+
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


// global PIT tick count
extern uint64_t g_pit_ticks;


// number of tasks that have been created
uint64_t g_task_count = 0;

// the initial memory pointed to by the current task pointer
k_task g_primer_task;

// currently task
k_task* g_current_task = &g_primer_task;

// global task list
k_task* g_task_list = NULL;


// Standard I/O streams.
// TODO: implement the ability for each process to ahve their own.
static FILE* current_stdin;
static FILE* current_stdout;
static FILE* current_stderr;

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


void k_task_init()
{
  current_stdin = (FILE*)k_heap_alloc(sizeof(FILE));
  if (current_stdin == NULL)
  {
    fprintf(stddbg, "[ERROR] failed to create stdin\n");
    HANG();
  }
  current_stdin->info = (void*)k_file_create_info(__FILE_NO_STDIN);
  if (current_stdin->info == NULL)
  {
    fprintf(stddbg, "[ERROR] failed to create stdin internal structure\n");
    HANG();
  }

  current_stdout = (FILE*)k_heap_alloc(sizeof(FILE));
  if (current_stdout == NULL)
  {
    fprintf(stddbg, "[ERROR] failed to create stdout\n");
    HANG();
  }
  current_stdout->info = (void*)k_file_create_info(__FILE_NO_STDOUT);
  if (current_stdout->info == NULL)
  {
    fprintf(stddbg, "[ERROR] failed to create stdout internal structure\n");
    HANG();
  }

  current_stderr = (FILE*)k_heap_alloc(sizeof(FILE));
  if (current_stderr == NULL)
  {
    fprintf(stddbg, "[ERROR] failed to create stderr\n");
    HANG();
  }
  current_stderr->info = (void*)k_file_create_info(__FILE_NO_STDERR);
  if (current_stderr->info == NULL)
  {
    fprintf(stddbg, "[ERROR] failed to create stderr internal structure\n");
    HANG();
  }
}

static void print_tasks()
{
  k_task* first = g_task_list;
  k_task* t = g_task_list;

  fprintf(stddbg, "(%llu,%d)", first->id, first->status);

  if (first->next == NULL)
  {
    return;
  }

  t = first->next;
  while (t != first)
  {
    fprintf(stddbg, " (%llu,%d)", t->id, t->status);
    t = t->next;
  }
  fprintf(stddbg, "\n");
}

k_regn* k_task_switch(k_regn* reg_stack)
{
  // If there are no tasks, then proceed with the current task.
  if (g_task_list == NULL)
  {
    return reg_stack;
  }

  // Save the register stack of the current task.
  g_current_task->regs = reg_stack;

  // Select the next task.
  // Currently, we just do round robin, which is really inefficient.
  if (g_current_task->next != NULL)
  {
    g_current_task = g_current_task->next;

    // print_tasks();

    // Attempt to find the next task wit a status of RUNNING.
    while (g_current_task->next != NULL
      && g_current_task->status != TASK_RUNNING)
    {
      // For tasks with a status of SLEEPING, check the
      // wake condition to see if they can changed to RUNNING.
      if (g_current_task->status == TASK_SLEEPING)
      {
        // Locks
        // wake condition: sync_val == 0
        if (g_current_task->sync_type == TASK_SYNC_LOCK
          && *g_current_task->sync_val == 0)
        {
          g_current_task->status = TASK_RUNNING;
        }

        // Semaphores
        // wake condition: sync_val > 0
        else if (g_current_task->sync_type == TASK_SYNC_SEMAPHORE
          && (int64_t)*g_current_task->sync_val > 0)
        {
          g_current_task->status = TASK_RUNNING;
        }

        // Ticks
        // wake condition: g_pit_ticks - ticks >= limit
        else if (g_current_task->sync_type == TASK_SYNC_TICK
          && g_pit_ticks - g_current_task->ticks >= g_current_task->limit)
        {
          g_current_task->status = TASK_RUNNING;
        }

        else
        {
          g_current_task = g_current_task->next;
        }
      }

      // For tasks with a status of STOPPED, remove
      // them from the list.
      else if (g_current_task->status == TASK_STOPPED)
      {
        k_task* target = g_current_task;
        g_current_task = g_current_task->next;
        remove_task(target);
        target->status = TASK_REMOVED;
      }
      else
      {
        g_current_task = g_current_task->next;
      }
    }
  }

  // Return the register stack of the next task.
  return g_current_task->regs;
}


k_task* k_task_create(void (action)())
{
  void* task_mem; // task memory
  k_regn rsp;   // stack pointer
  k_regn rbp;   // base pointer

  // Allocate 16 Kib of stack space, and 4 Kib for the task state.
  // The task memory will have the following layout:
  // +--------------------+
  // |      4 Kib         |
  // |   task state and   | <- initial register stack
  // |  register values   | <- task state
  // |--------------------| <- RBP
  // |                    | <- padding
  // |       16 Kib       | <- RSP
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
  rbp = PTR_TO_N(task_mem) + 0x4000;
  rsp = rbp - TASK_STACK_SPACE;


  // Put the address of the k_syscall_stop function on the top of the stack.
  *(k_regn*)(rsp) = PTR_TO_N(k_syscall_stop);

  // The memory that will hold the task state will start
  // at at an offset of 16 bytes from the start of the fifth page.
  k_task* task = (k_task*)(rbp + 0x10);

  // The register stack memory will start at at an offset of 240 bytes
  // from the start of the fifth page.
  // This leaves sufficient space between the task state memory and
  // the initial register stack.
  // This address must be a multiple of 16.
  task->regs = (k_regn*)(rbp + 0xF0);


  // Build an ISR stack whose values will be popped
  // off the stack by the IRET instruction.
  task->regs[TASK_REG_SS] = 0x10;                // data segment
  task->regs[TASK_REG_RSP] = rsp;                // stack pointer
  task->regs[TASK_REG_RFLAGS] = 0x200;           // bit 9 (interrupt flag)
  task->regs[TASK_REG_CS] = 0x8;                 // code segment.
  task->regs[TASK_REG_RIP] = PTR_TO_N(action);   // start of execution


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
    t->status = TASK_RUNNING;
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
    t->status = TASK_RUNNING;
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

k_regn* k_task_stop(k_regn* regs)
{
  g_current_task->status = TASK_STOPPED;

  return k_task_switch(regs);
}

k_regn* k_task_sleep(k_regn* regs, k_regn* val, k_regn typ, k_regn ticks)
{
  // Set the synchronization value and type in the current task,
  // then set the current task's status to SLEEPING.
  g_current_task->sync_val = val;
  g_current_task->sync_type = typ;
  g_current_task->ticks = g_pit_ticks;
  g_current_task->limit = ticks;

  g_current_task->status = TASK_SLEEPING;

  return k_task_switch(regs);
}

void* k_task_get_io_buffer(int type)
{
  switch (type)
  {
  case __FILE_NO_STDIN:
    return current_stdin;

  case __FILE_NO_STDOUT:
    return current_stdout;

  case __FILE_NO_STDERR:
    return current_stderr;

  default:
    return NULL;
  }
}
