#include "osdev64/firmware.h"
#include "osdev64/core.h"
#include "osdev64/memory.h"

#include "klibc/stdio.h"


// global system memory map
extern k_mem_map g_sys_mem;


/**
 * A single entry in the physical RAM pool
 */
typedef struct pool_entry {
  k_regn address; // physical address
  k_regn pages;   // number of pages
}pool_entry;

/**
 * A single entry in the page reservation ledger.
 */
typedef struct ledger_entry {
  int i; // index of available memory region array
  k_regn address;
  k_regn pages;
  k_byte avail; // availability flag
}ledger_entry;

// number of regions in the RAM pool
int g_pool_count = 0;

// RAM pool
pool_entry g_ram_pool[RAM_POOL_MAX];

// RAM ledger
ledger_entry* g_ram_ledger;

uint64_t g_total_ram = 0;


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
  fprintf(
    stddbg,
    "map size: %lu, key: %lu, descriptor"
    " size: %lu, descriptor version: %lu\n",
    g_sys_mem.map_size,
    g_sys_mem.key,
    g_sys_mem.desc_size,
    g_sys_mem.version
  );

  EFI_MEMORY_DESCRIPTOR* d;
  char* start = (char*)(g_sys_mem.buffer);
  char* end = start + g_sys_mem.map_size;

  for (; start + g_sys_mem.desc_size <= end; start += g_sys_mem.desc_size)
  {
    d = (EFI_MEMORY_DESCRIPTOR*)start;
    fprintf(
      stddbg,
      "%-20s %p %ld \n",
      efi_mem_str(d->Type),
      d->PhysicalStart,
      d->NumberOfPages
    );
  }
}


void k_memory_init()
{
  ledger_entry root;        // root page reservation
  EFI_MEMORY_DESCRIPTOR* d; // UEFI memory descriptor
  char* start;              // base address of memory map
  char* end;                // end of memory map

  // Get the memory map from the firmware.

  // Fill the RAM pool.
  start = (char*)(g_sys_mem.buffer);
  end = start + g_sys_mem.map_size;

  for (; start + g_sys_mem.desc_size <= end; start += g_sys_mem.desc_size)
  {
    d = (EFI_MEMORY_DESCRIPTOR*)start;

    g_total_ram += d->NumberOfPages * 0x1000;

    // The memory type should be either EfiConventionalMemory,
    // EfiBootServicesCode, EfiBootServicesData,
    // EfiLoaderCode, or EfiLoaderData.
    if ((
      d->Type == EfiConventionalMemory
      || d->Type == EfiBootServicesCode
      || d->Type == EfiBootServicesData
      || d->Type == EfiLoaderCode
      || d->Type == EfiLoaderData
      )
      && g_pool_count < RAM_POOL_MAX)
    {
      g_ram_pool[g_pool_count].address = (uint64_t)(d->PhysicalStart);
      g_ram_pool[g_pool_count].pages = (uint64_t)(d->NumberOfPages);

      // Don't allow an entry at address 0.
      // If the base of the region is 0, attempt to use
      // 0x1000 as the base address instead.
      if (g_ram_pool[g_pool_count].address == 0)
      {
        if (g_ram_pool[g_pool_count].pages > 1)
        {
          g_ram_pool[g_pool_count].address += 0x1000;
          g_ram_pool[g_pool_count].pages--;
          g_pool_count++;
        }
      }
      else
      {
        g_pool_count++;
      }
    }
  }

  // Terminate UEFI boot services.
  // We do this immediately after obtaining the memory map
  // so that we can use the regions marked as BootServiceCode,
  // BootServiceData, etc.
  if (!k_firmware_exit())
  {
    fprintf(stddbg, "[ERROR] Failed to exit UEFI boot services.\n");
    HANG();
  }

  // Round the total RAM up to the nearest GiB to account for holes.
  while (g_total_ram % 0x40000000)
  {
    g_total_ram += 0x1000;
  }

  // Ensure that all RAM pool entries are 4KiB aligned.
  // If they're not, then we should fail here.
  for (int i = 0; i < g_pool_count; i++)
  {
    if (g_ram_pool[i].address % 0x1000)
    {
      fprintf(stddbg, "[ERROR] RAM pool entry not 4K aligned\n");
      HANG();
    }
  }

  // Sort the memory regions in ascending order of size
  // so smaller regions are used first for smaller allocations.
  // We use bubble sort since this code only runs once and doesn't
  // need to be particularly fast. We're probably not ever going
  // to have a million entries in the pool.
  int sorted = 0;
  while (!sorted)
  {
    sorted = 1;
    for (int i = 0; i < g_pool_count; i++)
    {
      if (g_ram_pool[i].pages > g_ram_pool[i + 1].pages
        && i < g_pool_count - 1
        )
      {
        sorted = 0;
        pool_entry tmp = g_ram_pool[i];
        g_ram_pool[i] = g_ram_pool[i + 1];
        g_ram_pool[i + 1] = tmp;
      }
    }
  }

  // Create the page reservation ledger.
  for (int i = 0; i < g_pool_count; i++)
  {
    // Look for a region that contains at least 41 pages.
    // Currently, the RAM_LEDGER_MAX is 5000, and each ledger
    // entry is assumed to be 32 bytes, so 5000 entries would
    // take up 40 pages. We use page 41 for the system font data.
    // We use pages 42 and 43 for the temporary loaded app.
    if (g_ram_pool[i].pages >= 43)
    {
      // Populate the root memory reservation.
      root.i = i;
      root.address = g_ram_pool[i].address;
      root.pages = 43;
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

  fprintf(stddbg, "[ERROR] failed to allocate %llu pages\n", n);

  return NULL;
}


void k_memory_free_pages(void* addr)
{
  k_regn a = PTR_TO_N(addr);

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
    fprintf(stddbg, "| %p %16llu |\n", g_ram_pool[i].address, g_ram_pool[i].pages);
  }

  fprintf(stddbg, "+-----------------------------------+\n");
}


void k_memory_print_ledger()
{
  fprintf(stddbg, "+------------------------------------------------------+\n");
  fprintf(stddbg, "| start             end               pages            |\n");
  fprintf(stddbg, "+------------------------------------------------------+\n");

  for (int i = 0; i < RAM_LEDGER_MAX; i++)
  {
    if (!g_ram_ledger[i].avail)
    {
      fprintf(stddbg,
        "| %p  %p  %-16llu |\n",
        g_ram_ledger[i].address,
        g_ram_ledger[i].address + (g_ram_ledger[i].pages * 0x1000) - 1,
        g_ram_ledger[i].pages
      );
    }
  }

  fprintf(stddbg, "+------------------------------------------------------+\n");
}
