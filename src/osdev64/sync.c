#include "osdev64/sync.h"
#include "osdev64/instructor.h"
#include "osdev64/memory.h"

#include "klibc/stdio.h"


// NOTE:
// Intel's manual contains the following warning about semaphores:
// "Do not implement semaphores using the WC memory type"
// Intel also recommends placing sempahores on 128-byte boundaries.



// memory used for storing synchronization values
static k_regn* sync_memory;

// bitmap for keeping track of synchronization value allocation
static uint64_t sync_bitmap[8];

// b should range from 0 to 511
static uint64_t check_bit(uint64_t bit)
{
  uint64_t i = bit / 64;
  uint64_t b = bit % 64;

  return sync_bitmap[i] & ((uint64_t)1 << b);
}

static uint64_t set_bit(uint64_t bit)
{
  uint64_t i = bit / 64;
  uint64_t b = bit % 64;

  sync_bitmap[i] |= ((uint64_t)1 << b);
}

static uint64_t clear_bit(uint64_t bit)
{
  uint64_t i = bit / 64;
  uint64_t b = bit % 64;

  sync_bitmap[i] &= ~((uint64_t)1 << b);
}

void k_sync_init()
{
  // Allocate 4 Kib for the memory pool.
  // If we use 8 bytes for each synchronization value, that allows
  // for up to 512 unique values to exist simultaneously.
  sync_memory = (k_regn*)k_memory_alloc_pages(1);

  if (sync_memory == NULL)
  {
    fprintf(
      stddbg,
      "[ERROR] failed to allocate memory for synchronization values\n"
    );
    HANG();
  }
}


k_lock* k_mutex_create()
{
  // Find the first available bit in the bitmap.
  for (uint64_t b = 0; b < 512; b++)
  {
    if (!check_bit(b))
    {
      sync_memory[b] = 0;
      set_bit(b);
      return &sync_memory[b];
    }
  }

  return NULL;
}


void k_mutex_destroy(k_lock* sl)
{
  // Clear the corresponding bit in the bitmap.
  for (uint64_t b = 0; b < 512; b++)
  {
    if (&sync_memory[b] == sl)
    {
      clear_bit(b);
      return;
    }
  }
}


void k_mutex_acquire(k_lock* sl, int busy)
{
  if (busy)
  {
    k_lock_spin(sl);
  }
  else
  {
    k_lock_sleep(sl);
  }
}


void k_mutex_release(k_lock* sl)
{
  k_btr(0, sl);
}



k_semaphore* k_semaphore_create(int64_t n)
{
  // Find the first available bit in the bitmap.
  for (uint64_t b = 0; b < 512; b++)
  {
    if (!check_bit(b))
    {
      sync_memory[b] = n;
      set_bit(b);
      return &sync_memory[b];
    }
  }

  return NULL;
}


void k_semaphore_destroy(k_semaphore* s)
{
  // Clear the corresponding bit in the bitmap.
  for (uint64_t b = 0; b < 512; b++)
  {
    if (&sync_memory[b] == s)
    {
      clear_bit(b);
      return;
    }
  }
}


void k_semaphore_wait(k_semaphore* s, int busy)
{
  if (busy)
  {
    k_sem_wait(s);
  }
  else
  {
    k_sem_sleep(s);
  }
}


void k_semaphore_signal(k_semaphore* s)
{
  k_xadd(1, s);
}
