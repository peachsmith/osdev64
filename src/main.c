#include "core.h"

#include "serial.h"
#include "descriptor.h"
#include "acpi.h"


void k_disable_interrupts();
void k_enable_interrupts();

// attempts to do something that results in an exception
void k_cause_exception();






/**
 * Attempts to get the UEFI graphics output protocol.
 *
 * Params:
 *   EFI_SYSTEM_TABLE* - a pointer to the UEFI system table
 *   EFI_SERIAL_IO_PROTOCOL** - a pointer to a pointer to a serial IO protocol struct
 *
 * Returns:
 *   EFI_STATUS - EFI_SUCCESS on success, otherwise an error code
 */
EFI_STATUS k_get_graphics(EFI_SYSTEM_TABLE* systab, EFI_GRAPHICS_OUTPUT_PROTOCOL** gop)
{
  EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

  return uefi_call_wrapper(systab->BootServices->LocateProtocol, 3, &gop_guid, NULL, (void**)gop);
}


static WCHAR* wc_PixelRedGreenBlueReserved8BitPerColor = L"RGB 8";
static WCHAR* wc_PixelBlueGreenRedReserved8BitPerColor = L"BGR 8";
static WCHAR* wc_PixelBitMask = L"bit mask";
static WCHAR* wc_PixelBltOnly = L"BLT";
static WCHAR* wc_PixelInvalid = L"Invalid";

static WCHAR* get_pixel_format_str(EFI_GRAPHICS_PIXEL_FORMAT fmt)
{
  switch (fmt)
  {
  case PixelRedGreenBlueReserved8BitPerColor:
    return wc_PixelRedGreenBlueReserved8BitPerColor;

  case PixelBlueGreenRedReserved8BitPerColor:
    return wc_PixelBlueGreenRedReserved8BitPerColor;

  case PixelBitMask:
    return wc_PixelBitMask;

  case PixelBltOnly:
    return wc_PixelBltOnly;

  default:
    return wc_PixelInvalid;
  }
}

#define BGR8_PIXEL(r,g,b) (((uint32_t)b) | ((uint32_t)g << 8) | ((uint32_t)r << 16))

/**
 * Writes pixel data to video memory.
 */
void k_put_pixel(
  uint64_t x,
  uint64_t y,
  uint8_t r,
  uint8_t g,
  uint8_t b,
  EFI_GRAPHICS_OUTPUT_PROTOCOL* gop,
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* mode
)
{
  // Currently, we only use BGR8
  if (mode->PixelFormat != PixelBlueGreenRedReserved8BitPerColor)
  {
    return;
  }

  // Get the base address of the framebuffer
  volatile uint32_t* volatile base = (volatile uint32_t*)gop->Mode->FrameBufferBase;

  // Calculate the offset.
  uint64_t offset = (x + y * mode->PixelsPerScanLine);

  // Calculate the destination address of the pixel.
  base += offset;

  // Write the pixel to the framebuffer.
  *base = BGR8_PIXEL(r, g, b);
}

/**
 * Draws a line on the screen.
 */
void k_draw_line(
  int64_t x1,
  int64_t y1,
  int64_t x2,
  int64_t y2,
  uint8_t r,
  uint8_t g,
  uint8_t b,
  EFI_GRAPHICS_OUTPUT_PROTOCOL* gop,
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* mode
)
{
  // Currently, we only use BGR8
  if (mode->PixelFormat != PixelBlueGreenRedReserved8BitPerColor)
  {
    return;
  }

  int64_t x = x1;
  int64_t y = y1;

  int64_t dx = x2 - x1;
  int64_t dy = y2 - y1;

  int64_t adx = dx >= 0 ? dx : -dx;
  int64_t ady = dy >= 0 ? dy : -dy;
  int64_t limit = ady;
  int64_t n = 0;

  int64_t p = 2 * dy - dx;

  // If the x coordinate doesn't change, draw a vertical line.
  if (dx == 0)
  {
    while (y < y2)
    {
      k_put_pixel(x, y, r, g, b, gop, mode);
      y++;
    }

    return;
  }

  int64_t c = dy / dx;

  // If the absolute value of the change in y is greater than the absolute
  // value of the change in x, then we use y as our limiter.
  if (c <= 1 && ady <= adx)
  {
    limit = adx;
  }

  while (n++ < limit)
  {
    k_put_pixel(x, y, r, g, b, gop, mode);

    if (limit == adx)
    {
      x += (dx > 0 ? 1 : -1);

      p += (2 * ady);

      if (p >= 0)
      {
        p -= (2 * adx);

        y += (dy > 0 ? 1 : -1);
      }
    }
    else
    {
      y += (dy > 0 ? 1 : -1);

      p += (2 * adx);

      if (p >= 0)
      {
        p -= (2 * ady);

        x += (dx > 0 ? 1 : -1);
      }
    }
  }
}

/**
 * Used for debugging graphics stuff.
 *
 * Params:
 *   EFI_GRAPHICS_OUTPUT_PROTOCOL* - pointer to UEFI graphics protocol
 */
void k_do_graphics(EFI_GRAPHICS_OUTPUT_PROTOCOL* gop)
{
  EFI_STATUS res;
  UINT32 i;
  UINTN size;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* gomi;

  UINT32 select = 0;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* mode;

  // Print(L"max modes: %u\n", gop->Mode->MaxMode);
  // Print(L"size of mode buffer: %lu\n", size);
  // Print(L"+----------------------------------------------+\n");
  // Print(L"|  mode | format |  pps   |    resolution      |\n");
  for (i = 0; i < gop->Mode->MaxMode; i++)
  {
    res = uefi_call_wrapper(gop->QueryMode, 4, gop, i, &size, &gomi);

    if (res != EFI_SUCCESS)
    {
      Print(L"failed to query graphics mode: %r\n", res);
    }
    else
    {
      // Print(
      //   L"| %-.6lu|%-.8s|%-.8u|%.8lu x %-.8lu |\n",
      //   i,
      //   get_pixel_format_str(gomi->PixelFormat),
      //   gomi->PixelsPerScanLine,
      //   gomi->HorizontalResolution,
      //   gomi->VerticalResolution
      // );

      // Select the graphics mode if it's 640 x 480 with BGR8 format
      if (gomi->HorizontalResolution == 640
        && gomi->VerticalResolution == 480
        && gomi->PixelFormat == PixelBlueGreenRedReserved8BitPerColor)
      {
        mode = gomi;
        select = i;
      }
    }
  }
  // Print(L"+----------------------------------------------+\n");

  // Attempt to set the graphics mode
  res = uefi_call_wrapper(gop->SetMode, 2, gop, select);
  if (res != EFI_SUCCESS)
  {
    Print(L"failed to set graphics mode: %r\n", res);
    return;
  }

  // slope == 0
  k_draw_line(300, 200, 400, 200, 50, 230, 100, gop, mode); // green

  // slope < 1
  k_draw_line(300, 210, 400, 211, 255, 80, 80, gop, mode); // red
  k_draw_line(300, 220, 400, 227, 255, 80, 80, gop, mode); // red
  k_draw_line(300, 230, 400, 250, 255, 80, 80, gop, mode); // red

  // slope == 1
  k_draw_line(300, 250, 400, 350, 240, 220, 80, gop, mode); // yellow

  // slope > 1
  k_draw_line(300, 280, 400, 381, 200, 100, 200, gop, mode); // magenta
  k_draw_line(300, 290, 400, 397, 200, 100, 200, gop, mode); // magenta
  k_draw_line(300, 300, 400, 420, 200, 100, 200, gop, mode); // magenta

  // vertical line
  k_draw_line(300, 320, 300, 460, 50, 150, 240, gop, mode); // blue

  // slope < 0
  k_draw_line(300, 180, 400, 150, 10, 180, 180, gop, mode); // teal


  k_draw_line(300, 170, 200, 140, 255, 120, 20, gop, mode); // orange
  k_draw_line(300, 160, 200, 60, 150, 0, 150, gop, mode);  // purple
  k_draw_line(300, 150, 200, 20, 120, 100, 0, gop, mode);  // brown

  k_draw_line(280, 270, 180, 300, 255, 120, 20, gop, mode); // orange
  k_draw_line(280, 260, 180, 360, 150, 0, 150, gop, mode);  // purple
  k_draw_line(280, 250, 180, 370, 120, 100, 0, gop, mode);  // brown
}




/**
 * Kernel entry point.
 */
EFI_STATUS
EFIAPI
efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE* systab)
{
  //==============================
  // BEGIN UEFI boot services

  // Initialize UEFI services.
  k_uefi_init(image, systab);

  // TODO: abstract the graphics stuff
  EFI_STATUS res;
  EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
  res = k_get_graphics(systab, &gop);
  if (res != EFI_SUCCESS)
  {
    Print(L"failed to get the graphics output protocol: %r\n", res);
    for (;;);
  }
  k_do_graphics(gop);

  // Print out some ACPI information.
  k_acpi_read();

  // Get the memory map.
  k_uefi_get_mem_map();

  // Terminate the boot services.
  k_uefi_exit();

  // END UEFI boot services
  //==============================

  // Initialize serial output for debugging.
  k_serial_com1_init();

  k_serial_com1_puts("UEFI boot services have been terminated.\n");

  // Disable interrupts.
  k_disable_interrupts();

  // Load the GDT.
  k_load_gdt();

  // Load the IDT.
  k_load_idt();

  // Enable interrupts.
  k_enable_interrupts();

  // Test an exception handler.
  // k_cause_exception();

  k_serial_com1_puts("Initialization complete.\n");

  // The main loop.
  for (;;);

  // We should never get here.
  return EFI_SUCCESS;
}
