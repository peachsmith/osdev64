#include "osdev64/heap.h"
#include "osdev64/memory.h"

#include "klibc/stdio.h"


// The initial heap used by the kernel.
static k_byte* s_heap;
static k_regn s_start;
static k_regn s_end;


// flags
#define AVAILABLE 0x1


// Describes a region of dynamic memory.
// The current expected size of this struct is 24 bytes.
typedef struct k_heap_header {
  struct k_heap_header* next; // 8 bytes
  k_regn size;                // 8 bytes
  k_byte flags;               // 1 byte
  // compiler padding         // 7 bytes
}k_heap_header;


void k_heap_init()
{
  // Allocate 128 KiB of contiguous memory for the initial kernel heap.
  s_heap = (k_byte*)k_memory_alloc_pages(32);

  if (s_heap == NULL)
  {
    fprintf(
      stddbg,
      "[ERROR] failed to allocate initial kernel heap\n"
    );
    HANG();
  }

  // Given a header struct h with an address of a, the starting
  // address can be calculated as
  // s = a + sizeof(h).
  s_start = PTR_TO_N(s_heap) + sizeof(k_heap_header);

  // Given a starting address of s, and a number of bytes available of n,
  // the ending address e can be calculated as
  // e = s + n - 1.
  s_end = s_start + 0x20000 - sizeof(k_heap_header) - 1;

  // The initial header will have a next pointer of NULL to indicate that
  // it's the last header in the list.
  ((k_heap_header*)s_heap)[0].next = NULL;

  // Since 128 KiB (or 0x20000 bytes) were allocated for the heap,
  // the initial size is 0x20000 - sizeof(k_heap_header)
  ((k_heap_header*)s_heap)[0].size = 0x20000 - sizeof(k_heap_header);

  ((k_heap_header*)s_heap)[0].flags = AVAILABLE;
}


void* k_heap_alloc(size_t n)
{
  k_heap_header* h = (k_heap_header*)s_heap;

  // Locate the first free region of memory.
  // The AVAILABLE flag must be set and the size of the region must be
  // >= the request amount.
  while (h->next != NULL && (h->size < n || !(h->flags & AVAILABLE)))
  {
    h = h->next;
  }

  // If we reached the end of heap memory and did not find
  // any available regions, then the allocation failed.
  if (h->next == NULL && (h->size < n || !(h->flags & AVAILABLE)))
  {
    return NULL;
  }

  // h now describes a free region of memory that be allocated.
  // Get the starting and ending addresses of the region.
  k_regn start = PTR_TO_N(h) + sizeof(k_heap_header);
  k_regn end = start + h->size - 1;

  // Create a new heap header after the region that has been located.
  // This is only necessary if there is enough memory remaining in
  // the requested region to allow for a new heap header describing
  // at least one byte of memory.
  if (h->size >= n + sizeof(k_heap_header) + 1)
  {
    k_heap_header* newh = (k_heap_header*)(start + n);

    newh->next = h->next != NULL ? h->next : NULL;
    newh->size = end - (PTR_TO_N(newh) + sizeof(k_heap_header)) + 1;
    newh->flags = AVAILABLE;

    h->size = n;
    h->next = newh;
  }

  // Clear the AVAILABLE flag of the allocated region to indicate
  // that it is now allocated.
  h->flags &= ~(AVAILABLE);
}


void k_heap_free()
{

}

// Prints a description of a region of memory based on information
// found in a heap header.
static inline void print_header(k_heap_header* h)
{
  // Given a header struct h with an address of a, the starting
  // address can be calculated as
  // s = a + sizeof(h).
  k_regn start = PTR_TO_N(h) + sizeof(k_heap_header);

  // Given a starting address of s and a size of z, the ending
  // address can be calculated as e = s + z - 1.
  k_regn end = start + h->size - 1;

  fprintf(
    stddbg,
    "| %-16llX %-16llX %-16llu                |\n",
    start,
    end,
    h->size
  );
}

void k_heap_print()
{
  if (s_heap == NULL)
  {
    fprintf(stddbg, "[HEAP] kernel heap is NULL\n");
    return;
  }

  k_heap_header* h = (k_heap_header*)s_heap;

  fprintf(stddbg, "+-------------------------------------------------------------------+\n");
  fprintf(stddbg, "| header size: %-3lld  base: %-16llX  limit: %-16llX |\n", sizeof(k_heap_header), PTR_TO_N(s_heap), s_end);
  fprintf(stddbg, "+-------------------------------------------------------------------+\n");
  fprintf(stddbg, "| start            end              size                            |\n");
  fprintf(stddbg, "+-------------------------------------------------------------------+\n");
  while (h->next != NULL)
  {
    print_header(h);
    h = h->next;
  }
  print_header(h);
  fprintf(stddbg, "+-------------------------------------------------------------------+\n");

}
