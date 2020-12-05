#include "core.h"


// combines three bytes into a 32-bit number representing a BGR pixel
#define BGR8_PIXEL(r,g,b) (((uint32_t)b) \
  | ((uint32_t)g << 8) \
  | ((uint32_t)r << 16))

// combines three bytes into a 32-bit number representing an RGB pixel
#define RGB8_PIXEL(r,g,b) (((uint32_t)r) \
  | ((uint32_t)g << 8) \
  | ((uint32_t)b << 16))


// The main graphics information
k_graphics g_graphics;



static WCHAR* wc_PixelRedGreenBlueReserved8BitPerColor = L"RGB 8";
static WCHAR* wc_PixelBlueGreenRedReserved8BitPerColor = L"BGR 8";
static WCHAR* wc_PixelBitMask = L"bit mask";
static WCHAR* wc_PixelBltOnly = L"BLT";
static WCHAR* wc_PixelInvalid = L"Invalid";

static inline WCHAR* get_pixel_format_str(EFI_GRAPHICS_PIXEL_FORMAT fmt)
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



void k_graphics_init()
{
  k_uefi_get_graphics(&g_graphics);
}


void k_put_pixel(uint64_t x, uint64_t y, uint8_t r, uint8_t g, uint8_t b)
{
  volatile uint32_t* volatile base;
  uint64_t offset;
  uint32_t color;

  // Get the base address of the framebuffer
  base = (volatile uint32_t*)g_graphics.base;

  // Calculate the offset.
  offset = (x + y * g_graphics.pps);

  // Calculate the destination address of the pixel.
  base += offset;

  // Determine the color value.
  switch (g_graphics.format)
  {
  case PixelBlueGreenRedReserved8BitPerColor:
    color = BGR8_PIXEL(r, g, b);
    break;

  case PixelRedGreenBlueReserved8BitPerColor:
    color = RGB8_PIXEL(r, g, b);
    break;

  default:
    return;
    break;
  }

  // Write the pixel to the framebuffer.
  *base = color;
}

void k_draw_line(int64_t x1, int64_t y1, int64_t x2, int64_t y2,
  uint8_t r, uint8_t g, uint8_t b)
{

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
      k_put_pixel(x, y, r, g, b);
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
    k_put_pixel(x, y, r, g, b);

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
