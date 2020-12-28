#include "osdev64/uefi.h"


#include "klibc/stdio.h"


// The UEFI application image
EFI_HANDLE g_image;


// The UEFI system table
EFI_SYSTEM_TABLE* g_systab;


// graphics information
k_graphics g_graphics;


// PC screen font for rendering text
unsigned char g_font[4096];


// memory map
k_mem_map g_mem_map;


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


/**
 * Gets the wide char string representation of an EFI memory type
 * suitable for printing with GNU-EFI's Print function.
 *
 * Params:
 *   uint64_t - a UEFI memory descriptor type
 */
static WCHAR* efi_mem_wstr(uint64_t t)
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


void k_uefi_init(EFI_HANDLE image, EFI_SYSTEM_TABLE* systab)
{
  InitializeLib(image, systab);

  g_image = image;
  g_systab = systab;

  Print(L"\n");
}


void k_uefi_exit()
{
  EFI_STATUS res = uefi_call_wrapper(g_systab->BootServices->ExitBootServices, 2, g_image, g_mem_map.key);
  if (res != EFI_SUCCESS)
  {
    Print(L"failed to exit UEFI boot services: %r\n", res);
    fprintf(stddbg, "failed to exit UEFI boot services\n");
    fprintf(stderr, "failed to exit UEFI boot services\n");

    return;
  }

  fprintf(stddbg, "UEFI boot services have been terminated.\n");
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
    g_mem_map.map_size = map_size;
    g_mem_map.buffer = buffer;
    g_mem_map.key = key;
    g_mem_map.desc_size = desc_size;
    g_mem_map.version = version;
  }
}


void k_uefi_get_graphics()
{
  EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
  EFI_STATUS res;
  EFI_LOCATE_PROTOCOL loc = g_systab->BootServices->LocateProtocol;
  EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
  UINTN size;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* gomi;
  UINT32 select = 0;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* mode = NULL;

  // Get the UEFI graphics output protocol.
  res = uefi_call_wrapper(loc, 3, &gop_guid, NULL, (void**)&gop);
  if (res != EFI_SUCCESS)
  {
    Print(L"failed to get graphics protocol: %r\n", res);
    return;
  }

  // Loop through all the available graphics modes.
  // TODO: figure out how to switch modes after calling ExitBootServices.
  for (UINTN i = 0; i < gop->Mode->MaxMode; i++)
  {
    res = uefi_call_wrapper(gop->QueryMode, 4, gop, i, &size, &gomi);

    if (res != EFI_SUCCESS)
    {
      Print(L"failed to query graphics mode: %r\n", res);
    }
    else
    {
      // The graphics mode must have a horizontal resolution
      // of at least 640 pixels, and a vertical resolution
      // of at least 480 pixels.
      // The pixel mode must be either BGR or RGB.
      if (
        gomi->HorizontalResolution >= 640
        && gomi->VerticalResolution >= 480
        && (gomi->PixelFormat == PixelBlueGreenRedReserved8BitPerColor
          || gomi->PixelFormat == PixelRedGreenBlueReserved8BitPerColor
          )
        && mode == NULL
        )
      {
        mode = gomi;
        select = i;
      }
    }
  }

  // Attempt to set the graphics mode
  res = uefi_call_wrapper(gop->SetMode, 2, gop, select);
  if (res != EFI_SUCCESS)
  {
    Print(L"failed to set graphics mode: %r\n", res);
    return;
  }

  g_graphics.format = mode->PixelFormat;
  g_graphics.width = mode->HorizontalResolution;
  g_graphics.height = mode->VerticalResolution;
  g_graphics.pps = mode->PixelsPerScanLine;
  g_graphics.base = gop->Mode->FrameBufferBase;
  g_graphics.size = gop->Mode->FrameBufferSize;
}


void k_uefi_get_font()
{
  EFI_STATUS res;
  EFI_GUID sfs_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* sfs;
  EFI_LOCATE_PROTOCOL loc = g_systab->BootServices->LocateProtocol;
  EFI_FILE* root;
  EFI_FILE* zap_file; // the font file
  UINTN size;
  char* buffer;

  // Get the simple file system protocol to give us access to files.
  res = uefi_call_wrapper(loc, 3, &sfs_guid, NULL, (void**)&sfs);
  if (res != EFI_SUCCESS)
  {
    Print(L"failed to locate simple file system protocol: %r\n", res);
    return;
  }

  // Open the root volume.
  res = uefi_call_wrapper(sfs->OpenVolume, 2, sfs, (void**)&root);
  if (res != EFI_SUCCESS)
  {
    Print(L"failed to open root volume: %r\n", res);
    return;
  }

  // Open the font file.
  res = uefi_call_wrapper(root->Open, 5, root, (void**)&zap_file, L"zap-vga16.psf", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
  if (res != EFI_SUCCESS)
  {
    Print(L"failed to open zap-vga16.psf: %r\n", res);
    return;
  }

  // Read the PSF1 header.
  size = 4;
  res = uefi_call_wrapper(zap_file->Read, 3, zap_file, &size, (void*)g_font);
  if (res != EFI_SUCCESS)
  {
    Print(L"failed to read header from zap-vga16.psf: %r\n", res);
    return;
  }

  // Read the glyph data.
  size = 4096;
  res = uefi_call_wrapper(zap_file->Read, 3, zap_file, &size, (void*)g_font);
  if (res != EFI_SUCCESS)
  {
    Print(L"failed to read glyph data from zap-vga16.psf: %r\n", res);
    return;
  }

  // Close the font file.
  res = uefi_call_wrapper(zap_file->Close, 1, zap_file);
  if (res != EFI_SUCCESS)
  {
    Print(L"failed to close zap-vga16.psf: %r\n", res);
    return;
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
