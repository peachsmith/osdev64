#include "osdev64/sync.h"
#include "osdev64/instructor.h"
#include "osdev64/memory.h"

#include "klibc/stdio.h"


// NOTE:
// Intel's manual contains the following warning about semaphores:
// "Do not implement semaphores using the WC memory type"
// Intel also recommends placing sempahores on 128-byte boundaries.



// memory used for storing locks and semaphores
static k_regn* sync_memory;

// bitmap for keeping track of which spinlocks exist
static uint64_t spinlock_bitmap[8];

// b should range from 0 to 511
static uint64_t check_bit(uint64_t bit)
{
  uint64_t i = bit / 64;
  uint64_t b = bit % 64;

  return spinlock_bitmap[i] & ((uint64_t)1 << b);
}

static uint64_t set_bit(uint64_t bit)
{
  uint64_t i = bit / 64;
  uint64_t b = bit % 64;

  spinlock_bitmap[i] |= ((uint64_t)1 << b);
}

static uint64_t clear_bit(uint64_t bit)
{
  uint64_t i = bit / 64;
  uint64_t b = bit % 64;

  spinlock_bitmap[i] &= ~((uint64_t)1 << b);
}

void k_sync_init()
{
  // Allocate 4 Kib for the spinlock memory pool.
  // If we use 8 bytes for each spinlock, that allows
  // for up to 512 unique spinlocks to exist simultaneously.
  sync_memory = (k_regn*)k_memory_alloc_pages(1);

  if (sync_memory == NULL)
  {
    fprintf(
      stddbg,
      "[ERROR] failed to allocate memory for spinlocks\n"
    );
    HANG();
  }
}


k_spinlock* k_spinlock_create()
{
  // Find the first available bit in the spinlock bitmap.
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


void k_spinlock_destroy(k_spinlock* sl)
{
  // Clear the corresponding bit in the spinlock bitmap.
  for (uint64_t b = 0; b < 512; b++)
  {
    if (&sync_memory[b] == sl)
    {
      clear_bit(b);
      return;
    }
  }
}


void k_spinlock_acquire(k_spinlock* sl)
{
  k_bts_spin(sl);
}

void k_lock_acquire(k_spinlock* sl)
{
  k_bts_sleep(sl);
}


void k_spinlock_release(k_spinlock* sl)
{
  k_btr(0, sl);
}



k_semaphore* k_semaphore_create(int64_t n)
{
  // Find the first available bit in the bitmap.
  for (uint64_t b = 0; b < 510; b++)
  {
    if (!check_bit(b) && !check_bit(b + 1) && !check_bit(b + 2))
    {
      sync_memory[b] = n;
      sync_memory[b + 1] = 0;
      sync_memory[b + 2] = 0;
      set_bit(b);
      set_bit(b + 1);
      set_bit(b + 2);
      return &sync_memory[b];
    }
  }

  return NULL;
}



void k_semaphore_destroy(k_semaphore* s)
{
  // Clear the corresponding bit in the bitmap.
  for (uint64_t b = 0; b < 510; b++)
  {
    if (&sync_memory[b] == s)
    {
      clear_bit(b);
      clear_bit(b + 1);
      clear_bit(b + 2);
      return;
    }
  }
}


void k_semaphore_wait(k_semaphore* s)
{
  k_spinlock_acquire((k_spinlock*)&s[1]);

  k_sem_wait(s);

  k_spinlock_release((k_spinlock*)&s[1]);
}

void k_semaphore_signal(k_semaphore* s)
{
  k_spinlock_acquire((k_spinlock*)&s[2]);

  k_xadd(1, s);

  k_spinlock_release((k_spinlock*)&s[2]);
}
