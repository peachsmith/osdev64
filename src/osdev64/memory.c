#include "osdev64/core.h"
#include "osdev64/uefi.h"
#include "osdev64/memory.h"

#include "klibc/stdio.h"

// memory map obtained from the firmware
extern k_mem_map g_mem_map;


/**
 * A single entry in the physical RAM pool
 */
typedef struct pool_entry {
  uint64_t address; // physical address
  uint64_t pages;   // number of pages
}pool_entry;

/**
 * A single entry in the page reservation ledger.
 */
typedef struct ledger_entry {
  int i; // index of available memory region array
  uint64_t address;
  uint64_t pages;
  unsigned char avail; // availability flag
}ledger_entry;


// number of regions in the RAM pool
int g_pool_count = 0;

// RAM pool
pool_entry g_ram_pool[RAM_POOL_MAX];

// RAM ledger
ledger_entry* g_ram_ledger;


// char string representations of UEFI memory types
static char* c_EfiReservedMemoryType = "Reserved";
static char* c_EfiLoaderCode = "Loader Code";
static char* c_EfiLoaderData = "Loader Data";
static char* c_EfiBootServicesCode = "Boot Service Code";
static char* c_EfiBootServicesData = "Boot Service Data";
static char* c_EfiRuntimeServicesCode = "Runtime Code";
static char* c_EfiRuntimeServicesData = "Runtime Data";
static char* c_EfiConventionalMemory = "Conventional";
static char* c_EfiUnusableMemory = "Unusable";
static char* c_EfiACPIReclaimMemory = "ACPI Reclaim";
static char* c_EfiACPIMemoryNVS = "ACPI NVS";
static char* c_EfiMemoryMappedIO = "MMIO";
static char* c_EfiMemoryMappedIOPortSpace = "MMIO Port";
static char* c_EfiPalCode = "Pal Code";
static char* c_EfiPersistentMemory = "Persistent";
static char* c_EfiMaxMemoryType = "Max Memory Type";
static char* c_EfiInvalid = "Invalid";


/**
 * Gets the string representation of an EFI memory type.
 *
 * Params:
 *   uint64_t - a UEFI memory descriptor type
 */
static char* efi_mem_str(uint64_t t)
{
  switch (t)
  {
  case EfiReservedMemoryType:
    return c_EfiReservedMemoryType;
    break;

  case EfiLoaderCode:
    return c_EfiLoaderCode;
    break;

  case EfiLoaderData:
    return c_EfiLoaderData;
    break;

  case EfiBootServicesCode:
    return c_EfiBootServicesCode;
    break;

  case EfiBootServicesData:
    return c_EfiBootServicesData;
    break;

  case EfiRuntimeServicesCode:
    return c_EfiRuntimeServicesCode;
    break;

  case EfiRuntimeServicesData:
    return c_EfiRuntimeServicesData;
    break;

  case EfiConventionalMemory:
    return c_EfiConventionalMemory;
    break;

  case EfiUnusableMemory:
    return c_EfiUnusableMemory;
    break;

  case EfiACPIReclaimMemory:
    return c_EfiACPIReclaimMemory;
    break;

  case EfiACPIMemoryNVS:
    return c_EfiACPIMemoryNVS;
    break;

  case EfiMemoryMappedIO:
    return c_EfiMemoryMappedIO;
    break;

  case EfiMemoryMappedIOPortSpace:
    return c_EfiMemoryMappedIOPortSpace;
    break;

  case EfiPalCode:
    return c_EfiPalCode;
    break;

    // NOTE: EfiPersistentMemory is a memory type in the current
    // version of the UEFI spec, but not in this version of GNU-EFI.
    // case EfiPersistentMemory:
    // return c_EfiPersistentMemory;
    // break;

  case EfiMaxMemoryType:
    return c_EfiMaxMemoryType;
    break;

  default:
    return c_EfiInvalid;
    break;
  }
}

/**
 * Prints the memory map
 * After calling this function, the memory map must be obtained
 * again before exiting the boot services.
 */
static void print_mmap()
{
  fprintf(stddbg, "map size: %lu, key: %lu, descriptor size: %lu, descriptor version: %lu\n",
    g_mem_map.map_size,
    g_mem_map.key,
    g_mem_map.desc_size,
    g_mem_map.version);

  EFI_MEMORY_DESCRIPTOR* d;
  void* start = (void*)(g_mem_map.buffer);
  void* end = start + g_mem_map.map_size;

  for (; start < end; start += g_mem_map.desc_size)
  {
    d = (EFI_MEMORY_DESCRIPTOR*)start;
    if (d->Type == EfiConventionalMemory && g_pool_count < RAM_POOL_MAX)
    {
      fprintf(stddbg, "%-20s %p %ld \n", efi_mem_str(d->Type), d->PhysicalStart, d->NumberOfPages);
    }
  }
}


void k_memory_init()
{
  ledger_entry root;        // root page reservation
  EFI_MEMORY_DESCRIPTOR* d; // UEFI memory descriptor
  void* start;              // base address of memory map
  void* end;                // end of memory map

  // Get the memory map from the firmware.
  k_uefi_get_mem_map();


  // Fill the RAM pool.
  start = (void*)(g_mem_map.buffer);
  end = start + g_mem_map.map_size;

  for (; start < end; start += g_mem_map.desc_size)
  {
    d = (EFI_MEMORY_DESCRIPTOR*)start;

    // The memory type should be EfiConventionalMemory and
    // we should have less than RAM_POOL_MAX regions collected.
    if (d->Type == EfiConventionalMemory && g_pool_count < RAM_POOL_MAX)
    {
      g_ram_pool[g_pool_count].address = (uint64_t)(d->PhysicalStart);
      g_ram_pool[g_pool_count++].pages = (uint64_t)(d->NumberOfPages);
    }
  }

  // Ensure that all RAM pool entries are 4KiB aligned.
  // If they're not, then we should fail here.
  for (int i = 0; i < g_pool_count; i++)
  {
    if (g_ram_pool[i].address % 0x1000)
    {
      fprintf(stddbg, "[ERROR] RAM pool entry not 4K aligned\n");
      for (;;);
    }
  }


  // Create the page reservation ledger.
  for (int i = 0; i < g_pool_count; i++)
  {
    // Look for a region that contains at least 8 pages.
    if (g_ram_pool[i].pages >= 8)
    {
      // Populate the root memory reservation.
      root.i = i;
      root.address = g_ram_pool[i].address;
      root.pages = 8;
      root.avail = 0;

      // Set the base address of the reservation array.
      g_ram_ledger = (ledger_entry*)(root.address);

      // Put the root memory reservation in the reservation array.
      g_ram_ledger[0] = root;

      // Mark all of the entries as available.
      for (int j = 1; j < RAM_LEDGER_MAX; j++)
      {
        g_ram_ledger[j].avail = 1;
      }

      return;
    }
  }
}


void* k_memory_alloc_pages(size_t n)
{
  for (int i = 0; i < g_pool_count; i++)
  {
    // Find a contiguous region of RAM that contains at least
    // the requested number of pages.
    if (g_ram_pool[i].pages >= n)
    {
      // number of addresses in the pool entry.
      uint64_t p_size = g_ram_pool[i].pages * 0x1000;

      // start and end addresses of requested memory region
      uint64_t req_start = g_ram_pool[i].address;
      uint64_t req_end = req_start + (n * 0x1000) - 1;

      for (int j = 0; j < RAM_LEDGER_MAX; j++)
      {
        if (g_ram_ledger[j].i == i && !g_ram_ledger[j].avail)
        {
          // start and end addresses of reserved memory region
          uint64_t res_start = g_ram_ledger[j].address;
          uint64_t res_end = res_start + (g_ram_ledger[j].pages * 0x1000) - 1;

          // If the requested region overlaps a reserved region,
          // update the start and end addresses of the requested region.
          if (
            (req_start >= res_start && req_start <= res_end)
            ||
            (res_start >= req_start && res_start <= req_end)
            )
          {
            req_start = res_end + 1;
            req_end = req_start + (n * 0x1000) - 1;
          }
        }
      }

      if (req_end <= g_ram_pool[i].address + p_size - 1)
      {
        // Create a new page reservation.
        ledger_entry res;
        res.i = i;
        res.avail = 0;
        res.address = req_start;
        res.pages = n;

        // Put the new page reservation in the ledger,
        // and return the base address of the reservation.
        for (int j = 0; j < RAM_LEDGER_MAX; j++)
        {
          if (g_ram_ledger[j].avail)
          {
            g_ram_ledger[j] = res;
            return (void*)(res.address);
          }
        }
      }
    }
  }

  return NULL;
}


void k_memory_free_pages(void* addr)
{
  uint64_t a = (uint64_t)addr;

  for (int i = 1; i < RAM_LEDGER_MAX; i++)
  {
    // Find the first entry in the page reservation ledger
    // that matches the address and mark it as available.
    // We skip the first entry in the ledger, since it is
    // the root page reservation.
    if (g_ram_ledger[i].address == a && !g_ram_ledger[i].avail)
    {
      g_ram_ledger[i].avail = 1;
      return;
    }
  }
}


void k_memory_print_pool()
{
  // Calculate the total amount of available RAM that we've gathered.
  uint64_t pages = 0;
  uint64_t bytes = 0;
  for (int i = 0; i < g_pool_count; i++)
  {
    pages += g_ram_pool[i].pages;
  }
  bytes = pages * 4096;

  fprintf(stddbg, "+-----------------------------------+\n");
  fprintf(stddbg, "| regions:                      %3d |\n", g_pool_count);
  fprintf(stddbg, "| pages:                 %10u |\n", pages);
  fprintf(stddbg, "| bytes:                 %10u |\n", bytes);
  fprintf(stddbg, "+-----------------------------------+\n");

  for (int i = 0; i < g_pool_count; i++)
  {
    fprintf(stddbg, "| %p %.16llu |\n", g_ram_pool[i].address, g_ram_pool[i].pages);
  }

  fprintf(stddbg, "+-----------------------------------+\n");
}


void k_memory_print_ledger()
{
  for (int i = 0; i < RAM_LEDGER_MAX; i++)
  {
    if (!g_ram_ledger[i].avail)
    {
      fprintf(stddbg,
        "%p %p %llu\n",
        g_ram_ledger[i].address,
        g_ram_ledger[i].address + (g_ram_ledger[i].pages * 0x1000) - 1,
        g_ram_ledger[i].pages
      );
    }
  }
}
