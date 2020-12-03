#include "core.h"

// The UEFI application image
EFI_HANDLE g_image;

// The UEFI system table
EFI_SYSTEM_TABLE* g_systab;

// memory map
k_mem_map mem_map;


// WCHAR string representations of UEFI memory types
static WCHAR* wc_EfiReservedMemoryType = L"Reserved";
static WCHAR* wc_EfiLoaderCode = L"Loader Code";
static WCHAR* wc_EfiLoaderData = L"Loader Data";
static WCHAR* wc_EfiBootServicesCode = L"Boot Service Code";
static WCHAR* wc_EfiBootServicesData = L"Boot Service Data";
static WCHAR* wc_EfiRuntimeServicesCode = L"Runtime Code";
static WCHAR* wc_EfiRuntimeServicesData = L"Runtime Data";
static WCHAR* wc_EfiConventionalMemory = L"Conventional";
static WCHAR* wc_EfiUnusableMemory = L"Unusable";
static WCHAR* wc_EfiACPIReclaimMemory = L"ACPI Reclaim";
static WCHAR* wc_EfiACPIMemoryNVS = L"ACPI NVS";
static WCHAR* wc_EfiMemoryMappedIO = L"MMIO";
static WCHAR* wc_EfiMemoryMappedIOPortSpace = L"MMIO Port";
static WCHAR* wc_EfiPalCode = L"Pal Code";
static WCHAR* wc_EfiPersistentMemory = L"Persistent";
static WCHAR* wc_EfiMaxMemoryType = L"Max Memory Type";
static WCHAR* wc_EfiInvalid = L"Invalid";

// Gets the WCHAR string representation of an EFI memory type
// suitable for printing with GNU-EFI's Print function.
static WCHAR* efi_mem_str(uint64_t t)
{
  switch (t)
  {
  case EfiReservedMemoryType:
    return wc_EfiReservedMemoryType;
    break;

  case EfiLoaderCode:
    return wc_EfiLoaderCode;
    break;

  case EfiLoaderData:
    return wc_EfiLoaderData;
    break;

  case EfiBootServicesCode:
    return wc_EfiBootServicesCode;
    break;

  case EfiBootServicesData:
    return wc_EfiBootServicesData;
    break;

  case EfiRuntimeServicesCode:
    return wc_EfiRuntimeServicesCode;
    break;

  case EfiRuntimeServicesData:
    return wc_EfiRuntimeServicesData;
    break;

  case EfiConventionalMemory:
    return wc_EfiConventionalMemory;
    break;

  case EfiUnusableMemory:
    return wc_EfiUnusableMemory;
    break;

  case EfiACPIReclaimMemory:
    return wc_EfiACPIReclaimMemory;
    break;

  case EfiACPIMemoryNVS:
    return wc_EfiACPIMemoryNVS;
    break;

  case EfiMemoryMappedIO:
    return wc_EfiMemoryMappedIO;
    break;

  case EfiMemoryMappedIOPortSpace:
    return wc_EfiMemoryMappedIOPortSpace;
    break;

  case EfiPalCode:
    return wc_EfiPalCode;
    break;

    // NOTE: EfiPersistentMemory is a memory type in the current
    // version of the UEFI spec, but not in this version of GNU-EFI.
    // case EfiPersistentMemory:
    // return wc_EfiPersistentMemory;
    // break;

  case EfiMaxMemoryType:
    return wc_EfiMaxMemoryType;
    break;


  default:
    return wc_EfiInvalid;
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
  Print(L"map size: %lu, key: %lu, descriptor size: %lu, descriptor version: %lu\n",
    mem_map.map_size,
    mem_map.key,
    mem_map.desc_size,
    mem_map.version
  );

  EFI_MEMORY_DESCRIPTOR* d;
  void* start = (void*)(mem_map.buffer);
  void* end = start + mem_map.map_size;

  for (;start < end; start += mem_map.desc_size)
  {
    d = (EFI_MEMORY_DESCRIPTOR*)start;
    Print(L" %-.20s %lX %ld \n", efi_mem_str(d->Type), d->PhysicalStart, d->NumberOfPages);
  }
}







void k_uefi_init(EFI_HANDLE image, EFI_SYSTEM_TABLE* systab)
{
  InitializeLib(image, systab);

  g_image = image;
  g_systab = systab;
}


void k_uefi_exit()
{
  uefi_call_wrapper(g_systab->BootServices->ExitBootServices, 2, g_image, mem_map.key);
}


void k_uefi_get_mem_map()
{
  // memory map data
  UINTN map_size = 4096;
  EFI_MEMORY_DESCRIPTOR* buffer;
  UINTN key = 0;
  UINTN desc_size = 0;
  UINT32 version = 0;

  // UEFI boot service functions
  EFI_ALLOCATE_POOL alloc_pool = g_systab->BootServices->AllocatePool;
  EFI_FREE_POOL free_pool = g_systab->BootServices->FreePool;
  EFI_GET_MEMORY_MAP get_map = g_systab->BootServices->GetMemoryMap;

  // UEFI status code
  EFI_STATUS res;


  // Allocate the initial buffer for the first attempt.
  res = uefi_call_wrapper(alloc_pool, 3, EfiLoaderData, map_size, (void**)&buffer);
  if (res != EFI_SUCCESS)
  {
    Print(L"failed to allocate initial memory map buffer: %r\n", res);
    return;
  }

  // Make the first attempt to get the memory map.
  res = uefi_call_wrapper(get_map, 5, &map_size, buffer, &key, &desc_size, &version);

  if (res != EFI_SUCCESS)
  {
    // If we encountered an error other than EFI_BUFFER_TOO_SMALL,
    // then we abandon the attempt.
    if (res != EFI_BUFFER_TOO_SMALL)
    {
      Print(L"failed to get the memory map: %r\n", res);
      return;
    }

    // Free the initial buffer.
    res = uefi_call_wrapper(free_pool, 1, buffer);

    if (res != EFI_SUCCESS)
    {
      Print(L"failed to free memory map buffer: %r\n", res);
      return;
    }

    // The first call to GetMemoryMap should have put the required buffer
    // size in this variable, so we add 1KiB to that to make sure we
    // have enough memory.
    map_size += 1024;

    // Allocate a new buffer with the required size.
    res = uefi_call_wrapper(alloc_pool, 3, EfiLoaderData, map_size, (void**)&buffer);

    if (res != EFI_SUCCESS)
    {
      Print(L"failed to allocate second memory map buffer: %r\n", res);
      return;
    }

    // Make a second attempt to get the memory map.
    res = uefi_call_wrapper(get_map, 5, &map_size, buffer, &key, &desc_size, &version);

    if (res != EFI_SUCCESS)
    {
      Print(L"failed to get the memory map on the second attempt: %r\n", res);
      return;
    }
  }

  // If we successfully obtained the memory map,
  // populate the global memory map structure.
  if (res == EFI_SUCCESS)
  {
    mem_map.map_size = map_size;
    mem_map.buffer = buffer;
    mem_map.key = key;
    mem_map.desc_size = desc_size;
    mem_map.version = version;
  }
}

int k_uefi_get_rsdp(unsigned char** rsdp)
{
  UINTN ent = g_systab->NumberOfTableEntries;
  EFI_CONFIGURATION_TABLE* tab = g_systab->ConfigurationTable;

  // GUIDs that allow us to identify which vendor table contains the RSDP.
  EFI_GUID acpi_1_guid = ACPI_TABLE_GUID;
  EFI_GUID acpi_2_guid = ACPI_20_TABLE_GUID;

  // UEFI vendor tables
  VOID* acpi1 = NULL;
  VOID* acpi2 = NULL;
  VOID* chosen_table = NULL;

  int have_acpi2 = 0;

  // Search the UEFI configuration table for vendor tables
  // that match the ACPI GUIDs.
  for (UINTN i = 0; i < ent; i++, tab++)
  {
    if (!CompareGuid(&(tab->VendorGuid), &acpi_1_guid))
    {
      acpi1 = tab->VendorTable;
    }
    else if (!CompareGuid(&(tab->VendorGuid), &acpi_2_guid))
    {
      have_acpi2 = 1;
      acpi2 = tab->VendorTable;
    }
  }

  // ACPI version 2 is preferred.
  chosen_table = acpi2 != NULL ? acpi2 : acpi1;

  // If we couldn't find the RSDP, return a failure code of 0.
  if (chosen_table == NULL)
  {
    return 0;
  }

  // Convert the UEFI VendorTable into an RSDP base address.
  *rsdp = (unsigned char*)(chosen_table);

  // Return 2 if we're using ACPI version >= 2, otherwise return 1.
  return have_acpi2 ? 2 : 1;
}
