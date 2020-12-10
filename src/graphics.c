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




#define BGR8_RED (BGR_PIXEL(220, 20, 50))
#define BGR8_ORANGE (BGR_PIXEL(220, 180, 50))
#define BGR8_YELLOW (BGR_PIXEL(220, 220, 50))
#define BGR8_GREEN (BGR_PIXEL(20, 220, 50))
#define BGR8_BLUE (BGR_PIXEL(50, 20, 220))
#define BGR8_PURPLE (BGR_PIXEL(220, 20, 220))



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


void k_text_init()
{
  k_uefi_load_font();
}

uint64_t text_x = 0; // multiple of 8
uint64_t text_y = 0; // multiple of 16

static void draw_glyph(unsigned char* g)
{
  for (int i = 0; i < 16; i++)
  {
    for (int j = 0; j < 8; j++)
    {
      if ((g[i] >> (7 - j)) & 1)
      {
        k_put_pixel(text_x + j, text_y + i, 200, 200, 200);
      }
    }
  }

  if (text_x < 640)
  {
    text_x += 8;
  }
  else
  {
    text_x = 0;
    if (text_y < 384)
    {
      text_y += 16;
    }

  }
}


// TODO: implement more text output functionality.
void k_draw_string(char* str)
{
  unsigned char* font = k_uefi_get_font();

  while (*str != '\0')
  {
    draw_glyph(&(font[*str * 16]));
    str++;
  }
}


void k_text_test()
{
  unsigned char* font = k_uefi_get_font();

  k_draw_string("Hello, World!");
}



// draws a rectanlge
void draw_rect(
  int64_t x,
  int64_t y,
  int64_t w,
  int64_t h
)
{
  int64_t x0 = x;
  int64_t y0 = y;
  int64_t x1 = x + w;
  int64_t y1 = y + h;

  // The goal is to draw a rectangle by drawing lines that connect
  // the following points like so:
  //
  //     (x0, y0)      line 1      (x1, y0)
  //        +-------------------------+
  //        |                         |
  // line 4 |                         | line 2
  //        |                         |
  //        +-------------------------+
  //     (x0, y1)      line 3      (x1, y1)

  k_draw_line(x0, y0, x1, y0, 80, 120, 250); // line 1
  k_draw_line(x1, y0, x1, y1, 80, 120, 250); // line 2
  k_draw_line(x1, y1, x0, y1, 80, 120, 250); // line 3
  k_draw_line(x0, y1, x0, y0, 80, 120, 250); // line 4
}

// draws a filled rectanlge
void fill_rect(
  int64_t x,
  int64_t y,
  int64_t w,
  int64_t h
)
{
  int64_t x0 = x;
  int64_t y0 = y;
  int64_t x1 = x + w;
  int64_t y1 = y + h;

  draw_rect(x, y, w, h);

  for (int64_t i = y0 + 1; i < y1; i++)
  {
    k_draw_line(x0, i, x1, i, 80, 120, 250); // line 1
  }
}

// draws a triangle
void draw_tri(
  int64_t x1,
  int64_t y1,
  int64_t x2,
  int64_t y2,
  int64_t x3,
  int64_t y3
)
{
  k_draw_line(x1, y1, x2, y2, 80, 250, 120); // line 1
  k_draw_line(x2, y2, x3, y3, 80, 250, 120); // line 2
  k_draw_line(x3, y3, x1, y1, 80, 250, 120); // line 3
}



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


// determines if a point is in a triangle
int is_in_tri(
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

// draws a filled triangle
void fill_tri(
  int64_t x1,
  int64_t y1,
  int64_t x2,
  int64_t y2,
  int64_t x3,
  int64_t y3
)
{
  // First, determine the bounding box
  int64_t xb0 = min3(x1, x2, x3);
  int64_t yb0 = min3(y1, y2, y3);
  int64_t xb1 = max3(x1, x2, x3);
  int64_t yb1 = max3(y1, y2, y3);

  // Draw the bounding box
  draw_rect(xb0, yb0, xb1 - xb0, yb1 - yb0);

  // Scan lines
  for (int64_t i = yb0 + 1; i < yb1; i++)
  {
    for (int64_t j = xb0 + 1; j < xb1; j++)
    {
      if (is_in_tri(x1, y1, x2, y2, x3, y3, j, i))
      {
        k_put_pixel(j, i, 80, 250, 120);
      }
      else if (i & 1)
      {
        k_put_pixel(j, i, 200, 50, 80);
      }
    }
  }

  // draw the outline of the traingle
  k_draw_line(x1, y1, x2, y2, 80, 250, 120); // line 1
  k_draw_line(x2, y2, x3, y3, 80, 250, 120); // line 2
  k_draw_line(x3, y3, x1, y1, 80, 250, 120); // line 3
}


// geometric primitive test
void k_geo_test()
{
  draw_rect(50, 50, 50, 50);

  fill_rect(50, 150, 50, 50);

  draw_tri(
    150, 50,
    175, 100,
    125, 100
  );

  fill_tri(
    150, 150,
    175, 200,
    125, 200
  );

  fill_tri(
    400, 220,
    420, 300,
    470, 100
  );

  fill_tri(
    300, 400,
    280, 380,
    290, 370
  );
}

