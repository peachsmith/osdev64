#include "osdev64/firmware.h"


// UEFI application image
static EFI_HANDLE sys_image;

// UEFI system table
// used for accessing UEFI boot services
static EFI_SYSTEM_TABLE* sys_tab;

// default system font
// needed for printing basic text
// Expected size: 4096
k_byte* g_sys_font;

// binary application code used to test loading executables
k_byte* g_app_bin;

// graphics information
// needed to output information to the screen
k_graphics g_sys_graphics;

// memory map
// needed to handle virtual and dynamic memory
k_mem_map g_sys_mem;

// RSDP
k_byte* g_sys_rsdp;

// ACPI version
int g_sys_acpi_ver = 0;



static void get_font()
{
  EFI_STATUS res;
  EFI_GUID sfs_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* sfs;
  EFI_FILE* root;
  EFI_FILE* font_file; // the font file
  UINTN size;
  char* buffer;

  // Get the simple file system protocol to give us access to files.
  res = uefi_call_wrapper(
    sys_tab->BootServices->LocateProtocol,
    3,
    &sfs_guid,
    NULL,
    (void**)&sfs
  );

  if (res != EFI_SUCCESS)
  {
    UEFI_PANIC("failed to get file system protocol: %r\n", res);
  }

  // Open the root volume.
  res = uefi_call_wrapper(sfs->OpenVolume, 2, sfs, (void**)&root);

  if (res != EFI_SUCCESS)
  {
    UEFI_PANIC("failed to open root volume: %r\n", res);
  }

  // Open the font file.
  res = uefi_call_wrapper(
    root->Open,
    5,
    root,
    (void**)&font_file,
    L"zap-vga16.psf",
    EFI_FILE_MODE_READ,
    EFI_FILE_READ_ONLY
  );

  if (res != EFI_SUCCESS)
  {
    UEFI_PANIC("failed to open zap-vga16.psf: %r\n", res);
  }

  // Allocate memory for the font.
  res = uefi_call_wrapper(
    sys_tab->BootServices->AllocatePool,
    3,
    EfiLoaderData,
    4096,
    (void**)&g_sys_font
  );
  if (res != EFI_SUCCESS)
  {
    UEFI_PANIC(
      "failed to allocate memory"
      " for font: %r\n",
      res
    );
  }

  // Read the PSF1 header.
  size = 4;
  res = uefi_call_wrapper(
    font_file->Read,
    3,
    font_file,
    &size,
    (void*)g_sys_font
  );

  if (res != EFI_SUCCESS)
  {
    UEFI_PANIC("failed to read header from zap-vga16.psf: %r\n", res);
  }

  // Read the glyph data.
  size = 4096;
  res = uefi_call_wrapper(
    font_file->Read,
    3,
    font_file,
    &size,
    (void*)g_sys_font
  );

  if (res != EFI_SUCCESS)
  {
    UEFI_PANIC("failed to read data from zap-vga16.psf: %r\n", res);
  }

  // Close the font file.
  res = uefi_call_wrapper(font_file->Close, 1, font_file);

  if (res != EFI_SUCCESS)
  {
    UEFI_PANIC("failed to close zap-vga16.psf: %r\n", res);
  }
}

static void get_app()
{
  EFI_STATUS res;
  EFI_GUID sfs_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* sfs;
  EFI_FILE* root;
  EFI_FILE* app_file; // the app file
  UINTN size;
  char* buffer;

  // Get the simple file system protocol to give us access to files.
  res = uefi_call_wrapper(
    sys_tab->BootServices->LocateProtocol,
    3,
    &sfs_guid,
    NULL,
    (void**)&sfs
  );

  if (res != EFI_SUCCESS)
  {
    UEFI_PANIC("failed to get file system protocol: %r\n", res);
  }

  // Open the root volume.
  res = uefi_call_wrapper(sfs->OpenVolume, 2, sfs, (void**)&root);

  if (res != EFI_SUCCESS)
  {
    UEFI_PANIC("failed to open root volume: %r\n", res);
  }

  // Open the executable app.
  res = uefi_call_wrapper(
    root->Open,
    5,
    root,
    (void**)&app_file,
    L"app.bin",
    EFI_FILE_MODE_READ,
    EFI_FILE_READ_ONLY
  );

  if (res != EFI_SUCCESS)
  {
    UEFI_PANIC("failed to open app.bin: %r\n", res);
  }

  // Allocate memory for the app.
  res = uefi_call_wrapper(
    sys_tab->BootServices->AllocatePool,
    3,
    EfiLoaderData,
    8192,
    (void**)&g_app_bin
  );
  if (res != EFI_SUCCESS)
  {
    UEFI_PANIC(
      "failed to allocate memory"
      " for app: %r\n",
      res
    );
  }

  // Read the binary data.
  size = 8192;
  res = uefi_call_wrapper(
    app_file->Read,
    3,
    app_file,
    &size,
    (void*)g_app_bin
  );

  if (res != EFI_SUCCESS)
  {
    UEFI_PANIC("failed to read data from app.bin: %r\n", res);
  }

  // Close the app file.
  res = uefi_call_wrapper(app_file->Close, 1, app_file);

  if (res != EFI_SUCCESS)
  {
    UEFI_PANIC("failed to close app.bin: %r\n", res);
  }
}

// Determines if a graphics mode is usable.
static inline int is_useable(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* g)
{
  if (g->HorizontalResolution < 640 || g->VerticalResolution < 480)
  {
    return 0;
  }

  if (g->PixelFormat == PixelBlueGreenRedReserved8BitPerColor)
  {
    return 1;
  }

  if (g->PixelFormat == PixelRedGreenBlueReserved8BitPerColor)
  {
    return 1;
  }

  return 0;
}

// get graphics
static void get_graphics()
{
  EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
  EFI_STATUS res;
  EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
  UINTN size;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* gomi;
  UINT32 select = 0;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* mode = NULL;

  // Get the UEFI graphics output protocol.
  res = uefi_call_wrapper(
    sys_tab->BootServices->LocateProtocol,
    3,
    &gop_guid,
    NULL,
    (void**)&gop
  );

  if (res != EFI_SUCCESS)
  {
    UEFI_PANIC("failed to get graphics protocol: %r\n", res);
  }

  // Loop through all the available graphics modes.
  // TODO: figure out how to switch modes after calling ExitBootServices.
  for (UINTN i = 0; i < gop->Mode->MaxMode; i++)
  {
    res = uefi_call_wrapper(
      gop->QueryMode,
      4,
      gop,
      i,
      &size,
      &gomi
    );

    if (res != EFI_SUCCESS)
    {
      UEFI_PANIC("failed to query graphics mode: %r\n", res);
    }
    else
    {
      // The graphics mode must have a horizontal resolution
      // of at least 640 pixels, and a vertical resolution
      // of at least 480 pixels. The pixel mode must be
      // either BGR or RGB.
      if (is_useable(gomi) && mode == NULL)
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
    UEFI_PANIC("failed to set graphics mode: %r\n", res);
  }

  g_sys_graphics.format = mode->PixelFormat;
  g_sys_graphics.width = mode->HorizontalResolution;
  g_sys_graphics.height = mode->VerticalResolution;
  g_sys_graphics.pps = mode->PixelsPerScanLine;
  g_sys_graphics.base = gop->Mode->FrameBufferBase;
  g_sys_graphics.size = gop->Mode->FrameBufferSize;
}

static void get_rsdp()
{
  UINTN ent = sys_tab->NumberOfTableEntries;
  EFI_CONFIGURATION_TABLE* tab = sys_tab->ConfigurationTable;

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
    UEFI_PANIC("failed to find RSDP", EFI_NOT_FOUND);
  }

  // Convert the UEFI VendorTable into an RSDP base address.
  g_sys_rsdp = (k_byte*)(chosen_table);

  // Return 2 if we're using ACPI version >= 2, otherwise return 1.
  g_sys_acpi_ver = have_acpi2 ? 2 : 1;
}


static void get_memory()
{
  // memory map data
  UINTN map_size = 4096;
  EFI_MEMORY_DESCRIPTOR* buffer;
  UINTN key = 0;
  UINTN desc_size = 0;
  UINT32 version = 0;

  // UEFI boot service functions
  EFI_ALLOCATE_POOL alloc_pool = sys_tab->BootServices->AllocatePool;
  EFI_FREE_POOL free_pool = sys_tab->BootServices->FreePool;
  EFI_GET_MEMORY_MAP get_map = sys_tab->BootServices->GetMemoryMap;

  // UEFI status code
  EFI_STATUS res;

  // Allocate the initial buffer for the first attempt.
  res = uefi_call_wrapper(
    alloc_pool,
    3,
    EfiLoaderData,
    map_size,
    (void**)&buffer
  );
  if (res != EFI_SUCCESS)
  {
    UEFI_PANIC(
      "failed to allocate initial"
      " memory map buffer: %r\n",
      res
    );
  }

  // Make the first attempt to get the memory map.
  res = uefi_call_wrapper(
    get_map,
    5,
    &map_size,
    buffer,
    &key,
    &desc_size,
    &version
  );

  if (res != EFI_SUCCESS)
  {
    // If we encountered an error other than EFI_BUFFER_TOO_SMALL,
    // then we abandon the attempt.
    if (res != EFI_BUFFER_TOO_SMALL)
    {
      UEFI_PANIC("failed to get the memory map: %r\n", res);
    }

    // Free the initial buffer.
    res = uefi_call_wrapper(free_pool, 1, buffer);

    if (res != EFI_SUCCESS)
    {
      UEFI_PANIC("failed to free memory map buffer: %r\n", res);
    }

    // The first call to GetMemoryMap should have put the required buffer
    // size in this variable, so we add 1KiB to that to make sure we
    // have enough memory.
    map_size += 1024;

    // Allocate a new buffer with the required size.
    res = uefi_call_wrapper(
      alloc_pool,
      3,
      EfiLoaderData,
      map_size,
      (void**)&buffer
    );

    if (res != EFI_SUCCESS)
    {
      UEFI_PANIC(
        "[ERROR] failed to allocate"
        " second memory map buffer: %r\n",
        res
      );
    }

    // Make a second attempt to get the memory map.
    res = uefi_call_wrapper(
      get_map,
      5,
      &map_size,
      buffer,
      &key,
      &desc_size,
      &version
    );

    if (res != EFI_SUCCESS)
    {
      UEFI_PANIC(
        "failed to get the memory"
        " map on the second attempt: %r\n",
        res
      );
    }
  }

  // If we successfully obtained the memory map,
  // populate the global memory map structure.
  if (res == EFI_SUCCESS)
  {
    g_sys_mem.map_size = map_size;
    g_sys_mem.buffer = buffer;
    g_sys_mem.key = key;
    g_sys_mem.desc_size = desc_size;
    g_sys_mem.version = version;
  }
}




void k_firmware_init(EFI_HANDLE image, EFI_SYSTEM_TABLE* systab)
{
  InitializeLib(image, systab);

  sys_image = image;
  sys_tab = systab;

  get_font();

  get_app();

  get_graphics();

  get_rsdp();

  get_memory();
}


int k_firmware_exit()
{
  EFI_STATUS res = uefi_call_wrapper(
    sys_tab->BootServices->ExitBootServices,
    2,
    sys_image,
    g_sys_mem.key
  );

  if (res != EFI_SUCCESS)
  {
    Print(L"[ERROR] Error calling ExitBootServices: %r\n", res);
    return 0;
  }

  return 1;
}
