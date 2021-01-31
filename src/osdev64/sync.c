#include "osdev64/sync.h"
#include "osdev64/instructor.h"
#include "osdev64/memory.h"

#include "klibc/stdio.h"

// memory used for creating spinlocks
static k_regn* spinlock_memory;

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
  spinlock_memory = (k_regn*)k_memory_alloc_pages(1);

  if (spinlock_memory == NULL)
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
      spinlock_memory[b] = 0;
      set_bit(b);
      return &spinlock_memory[b];
    }
  }

  return NULL;
}


void k_spinlock_destroy(k_spinlock* sl)
{
  // Clear the corresponding bit in the spinlock bitmap.
  for (uint64_t b = 0; b < 512; b++)
  {
    if (&spinlock_memory[b] == sl)
    {
      clear_bit(b);
      return;
    }
  }
}


void k_spinlock_acquire(k_spinlock* sl)
{
  k_bts_spin(0, sl);
}


void k_spinlock_release(k_spinlock* sl)
{
  k_btr(0, sl);
}
