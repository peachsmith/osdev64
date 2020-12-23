#include "osdev64/uefi.h"
#include "osdev64/memory.h"

#include "klibc/stdio.h"


extern k_mem_map g_mem_map;


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
    fprintf(stddbg, "%-20s %p %ld \n", efi_mem_str(d->Type), d->PhysicalStart, d->NumberOfPages);
  }
}


void k_memory_init()
{
  k_uefi_get_mem_map();
  print_mmap();
}
