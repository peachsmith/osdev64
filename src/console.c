#include "core.h"
#include "console.h"
#include "graphics.h"


// console dimensions in pixels
#define CON_WIDTH 640
#define CON_HEIGHT 480


// glyph dimensions in pixels
#define GLYPH_WIDTH 8
#define GLYPH_HEIGHT 16


// font used when rendering text
unsigned char* g_font;


void k_console_init()
{
  g_font = k_uefi_load_font();
}


// coordinates of the next character to be written
uint64_t text_x = 0; // multiple of 8
uint64_t text_y = 0; // multiple of 16


/**
 * Renders a glyph to the screen.
 * The glyph is expected to be an array of 16 bytes, where each bit in
 * each byte indicates whether or not a pixel should be plotted.
 * If a bit is 1, a pixel is plotted.
 *
 * Params:
 *   unsigned char* - an array of bytes containing the glyph data
 *   uint64_t - the x coordinate of the top left of the glyph on the screen
 *   uint64_t - the y coordinate of the top left of the glyph on the screen
 *   int8_t - the red component of the colour
 *   int8_t - the gren component of the colour
 *   int8_t - the blue component of the colour
 */
static void draw_glyph(
  unsigned char* glyph,
  uint64_t x,
  uint64_t y,
  uint8_t r,
  uint8_t g,
  uint8_t b
)
{
  // Plot a pixel for each bit with a value of 1.
  for (int i = 0; i < GLYPH_HEIGHT; i++)
  {
    for (int j = 0; j < GLYPH_WIDTH; j++)
    {
      if ((glyph[i] >> (7 - j)) & 1)
      {
        k_put_pixel(x + j, y + i, r, g, b);
      }
    }
  }
}


void k_console_putc(char c)
{
  // Limit the number of lines.
  if (text_y >= CON_HEIGHT)
  {
    return;
  }

  uint8_t n = (uint8_t)c;

  if (n < 32 || n > 126)
  {
    // Handle newlines.
    if (n == 10)
    {
      text_x = 0;
      text_y += GLYPH_HEIGHT;
    }

    return;
  }

  // If we've reached the end of the line,
  // reset x and increment y.
  if (text_x >= CON_WIDTH)
  {
    text_x = 0;
    text_y += GLYPH_HEIGHT;
  }

  // Locate the glyph in the font data that can be used
  // to represent the character.
  unsigned char* glyph = &(g_font[(int)c * GLYPH_HEIGHT]);

  // Draw the character on the screen.
  draw_glyph(glyph, text_x, text_y, 200, 200, 200);

  // Increment x.
  if (text_x < CON_WIDTH)
  {
    text_x += 8;
  }
}

void k_console_puts(char* str)
{
  while (*str != '\0')
  {
    k_console_putc(*str);
    str++;
  }
}
