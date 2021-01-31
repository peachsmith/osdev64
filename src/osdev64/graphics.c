#include "osdev64/firmware.h"
#include "osdev64/paging.h"

#include "klibc/stdio.h"

// combines three bytes into a 32-bit number representing a BGR pixel
#define BGR8_PIXEL(r,g,b) (((uint32_t)b) \
  | ((uint32_t)g << 8) \
  | ((uint32_t)r << 16))

// combines three bytes into a 32-bit number representing an RGB pixel
#define RGB8_PIXEL(r,g,b) (((uint32_t)r) \
  | ((uint32_t)g << 8) \
  | ((uint32_t)b << 16))


// determines the minimum of three numbers
#define min3(n1, n2, n3) (         \
    (n1 <= n2 && n1 <= n3) ? n1    \
    : (n2 <= n1 && n2 <= n3 ? n2   \
      : (n3 <= n1 && n3 <= n2 ? n3 \
        : n1))                     \
    )

// determines the maximum of three numbers
#define max3(n1, n2, n3) (         \
    (n1 >= n2 && n1 >= n3) ? n1    \
    : (n2 >= n1 && n2 >= n3 ? n2   \
      : (n3 >= n1 && n3 >= n2 ? n3 \
        : n1))                     \
    )


// The main graphics information
extern k_graphics g_sys_graphics;

// virtual address of framebuffer
volatile uint32_t* volatile g_framebuffer;

// string representations of UEFI pixel formats
static WCHAR* wc_PixelRedGreenBlueReserved8BitPerColor = L"RGB 8";
static WCHAR* wc_PixelBlueGreenRedReserved8BitPerColor = L"BGR 8";
static WCHAR* wc_PixelBitMask = L"bit mask";
static WCHAR* wc_PixelBltOnly = L"BLT";
static WCHAR* wc_PixelInvalid = L"Invalid";

/**
 * Gets the string representation of a UEFI pixel format.
 * The returned string is a wide char string suitable for
 * printing with UEFI's console output interface.
 *
 * Params:
 *   EFI_GRAPHICS_PIXEL_FORMAT - a UEFI pixel format
 *
 * Returns:
 *   WCHAR* - a wide char string with the name of the pixel format
 */
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


/**
 * This function takes the coordinates of three vertices, A, B, and C,
 * and determines if the given point P lies with the area enclosed
 * by those three vertices.
 * This implementation expects that Bx < Cx < Ax.
 * TODO: accommodate this limitation in the k_fill_triangle function.
 *
 * Params:
 *   int64_t - the x coordinate of the first vertex
 *   int64_t - the y coordinate of the first vertex
 *   int64_t - the x coordinate of the second vertex
 *   int64_t - the y coordinate of the second vertex
 *   int64_t - the x coordinate of the third vertex
 *   int64_t - the y coordinate of the third vertex
 *   int64_t - the x coordinate of the point in question
 *   int64_t - the y coordinate of the point in question
 */
static int point_in_triangle(
  int64_t ax, int64_t ay,
  int64_t bx, int64_t by,
  int64_t cx, int64_t cy,
  int64_t px, int64_t py
)
{
  float w1;
  float w2;
  float w;

  int64_t cay = cy - ay;
  int64_t cax = cx - ax;
  int64_t bay = by - ay;
  int64_t bax = bx - ax;
  int64_t pay = py - ay;

  int64_t w1_num = (ax * cay) + (pay * cax) - (px * cay);
  int64_t w1_den = (bay * cax) - (bax * cay);

  w1 = w1_den != 0 ? (w1_num / (float)w1_den) : (float)w1_num;

  int64_t w2_num = py - ay - (w1 * bay);

  w2 = cay != 0 ? (w2_num / (float)cay) : (float)w2_num;

  return w1 >= 0 && w2 >= 0 && w1 + w2 <= 1;
}



void k_graphics_init()
{
  g_framebuffer = (volatile uint32_t*)g_sys_graphics.base;
}

void k_graphics_map_framebuffer()
{
  uint64_t fb_phys = g_sys_graphics.base; // physical base
  uint64_t fb_size = g_sys_graphics.size; // size of buffer
  uint64_t fb_end = fb_phys + fb_size;    // physical end

  // Map the physical address range to a virtual address range.
  uint64_t fb_virt = k_paging_map_range(fb_phys, fb_end);
  if (!fb_virt)
  {
    fprintf(stddbg, "[ERROR] failed to map framebuffer\n");
    for (;;);
  }

  // Update the framebuffer address.
  g_framebuffer = (volatile uint32_t*)fb_virt;
}

// uint64_t k_graphics_get_phys_base()
// {
//   return g_graphics.base;
// }


// uint64_t k_graphics_get_size()
// {
//   return (uint64_t)(g_graphics.size);
// }


// void k_graphics_set_virt_base(uint64_t base)
// {
//   g_framebuffer = (volatile uint32_t*)base;
// }


void k_put_pixel(uint64_t x, uint64_t y, uint8_t r, uint8_t g, uint8_t b)
{
  volatile uint32_t* volatile dest;
  uint32_t color;

  // Calculate the destination address of the pixel.
  // As a reminder, "pps" is "pixels per scanline".
  dest = g_framebuffer + (x + y * g_sys_graphics.pps);

  // Determine the color value.
  switch (g_sys_graphics.format)
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
  *dest = color;
}


void k_draw_line(
  int64_t x1, int64_t y1,
  int64_t x2, int64_t y2,
  uint8_t r, uint8_t g, uint8_t b
)
{
  // This function is based on the implementation of Bresenham's line
  // drawing algorithm found on the Xbox hobbyist site xbdev.net.
  // https://xbdev.net/non_xdk/openxdk/drawline/index.php


  // The net change in x and y from point 1 to point 2.
  int64_t dx = x2 - x1;
  int64_t dy = y2 - y1;

  // Counter variables to keep track of how many points we've plotted.
  int64_t y_count = 0;
  int64_t x_count = 0;

  // The amount by which the x and y values are incremented.
  // If the net change from point 1 to point is negative,
  // then this increment value will be -1, otherwise it will
  // be 1.
  int64_t x_inc = (dx < 0) ? -1 : 1;
  int64_t y_inc = (dy < 0) ? -1 : 1;

  // The limiting factor of the x and y coordinate is
  // the absolute value of the net change from point 1
  // to point 2. This is the minimum number of points
  // that need to be plotted to draw the complete line.
  dx = x_inc * dx + 1;
  dy = y_inc * dy + 1;

  // The x and y coordinates of each point along the line.
  int64_t px = x1;
  int64_t py = y1;

  // If the change in x is greater than or equal to the change in y,
  // then we increment or decrement the x cooridnate on every iteration,
  // otherwise we increment the y coordinate.
  if (dx >= dy)
  {
    for (x_count = 0; x_count < dx; x_count++)
    {
      k_put_pixel(px, py, r, g, b);

      y_count += dy;
      px += x_inc;

      if (y_count >= dx)
      {
        y_count -= dx;
        py += y_inc;
      }
    }
  }
  else
  {
    for (y_count = 0; y_count < dy; y_count++)
    {
      k_put_pixel(px, py, r, g, b);

      x_count += dx;
      py += y_inc;

      if (x_count >= dy)
      {
        x_count -= dy;
        px += x_inc;
      }
    }
  }
}


void k_draw_rect(
  int64_t x, int64_t y,
  int64_t w, int64_t h,
  uint8_t r, uint8_t g, uint8_t b
)
{
  int64_t x1 = x;
  int64_t y1 = y;
  int64_t x2 = x + w;
  int64_t y2 = y + h;

  k_draw_line(x1, y1, x2, y1, r, g, b); // line 1
  k_draw_line(x2, y1, x2, y2, r, g, b); // line 2
  k_draw_line(x2, y2, x1, y2, r, g, b); // line 3
  k_draw_line(x1, y2, x1, y1, r, g, b); // line 4
}


void k_fill_rect(
  int64_t x, int64_t y,
  int64_t w, int64_t h,
  uint8_t r, uint8_t g, uint8_t b
)
{
  int64_t x1 = x;
  int64_t y1 = y;
  int64_t x2 = x + w;
  int64_t y2 = y + h;

  k_draw_rect(x, y, w, h, r, g, b);

  for (int64_t i = y1 + 1; i < y2; i++)
  {
    k_draw_line(x1, i, x2, i, r, g, b);
  }
}


void k_draw_triangle(
  int64_t x1,
  int64_t y1,
  int64_t x2,
  int64_t y2,
  int64_t x3,
  int64_t y3,
  uint8_t r, uint8_t g, uint8_t b
)
{
  k_draw_line(x1, y1, x2, y2, r, g, b); // line 1
  k_draw_line(x2, y2, x3, y3, r, g, b); // line 2
  k_draw_line(x3, y3, x1, y1, r, g, b); // line 3
}


void k_fill_triangle(
  int64_t x1,
  int64_t y1,
  int64_t x2,
  int64_t y2,
  int64_t x3,
  int64_t y3,
  uint8_t r, uint8_t g, uint8_t b
)
{
  // First, determine the bounding box
  int64_t xb0 = min3(x1, x2, x3);
  int64_t yb0 = min3(y1, y2, y3);
  int64_t xb1 = max3(x1, x2, x3);
  int64_t yb1 = max3(y1, y2, y3);

  // Scan lines
  for (int64_t i = yb0 + 1; i < yb1; i++)
  {
    for (int64_t j = xb0 + 1; j < xb1; j++)
    {
      if (point_in_triangle(x1, y1, x2, y2, x3, y3, j, i))
      {
        k_put_pixel(j, i, r, g, b);
      }
    }
  }

  // draw the outline of the traingle
  k_draw_line(x1, y1, x2, y2, r, g, b); // line 1
  k_draw_line(x2, y2, x3, y3, r, g, b); // line 2
  k_draw_line(x3, y3, x1, y1, r, g, b); // line 3
}
