#ifndef JEP_GRAPHICS_H
#define JEP_GRAPHICS_H

#include <stdint.h>


/**
 * Initializes the graphics interface.
 * This must be called once before any of the other graphics functions
 * are called.
 */
void k_graphics_init();

/**
 * Gets the base physical address of the framebuffer.
 *
 * Returns:
 *   uint64_t - the base physical address of the framebuffer
 */
// uint64_t k_graphics_get_phys_base();

/**
 * Gets the size of the framebuffer in bytes.
 *
 * Returns:
 *   uint64_t - the size of the framebuffer in bytes
 */
// uint64_t k_graphics_get_size();


/**
 * Maps the base address of the framebuffer into virtual memory.
 * The virtual memory manager must be initialized before calling
 * this function.
 */
void k_graphics_map_framebuffer();

/**
 * Plots a single pixel on the screen.
 * The origin (0,0) is in the top leftr corner of the screen.
 *
 * Params:
 *   uint64_t - the x coordinate
 *   uint64_t - the y coordinate
 *   int8_t - the red component of the colour
 *   int8_t - the gren component of the colour
 *   int8_t - the blue component of the colour
 */
void k_put_pixel(
  uint64_t x, uint64_t y,
  uint8_t r, uint8_t g, uint8_t b
);


/**
 * Draws a line between two points.
 *
 * Params:
 *   int64_t - the x coordinate of the first point
 *   int64_t - the y coordinate of the first point
 *   int64_t - the x coordinate of the second point
 *   int64_t - the y coordinate of the second point
 *   int8_t - the red component of the colour
 *   int8_t - the gren component of the colour
 *   int8_t - the blue component of the colour
 */
void k_draw_line(
  int64_t x1, int64_t y1,
  int64_t x2, int64_t y2,
  uint8_t r, uint8_t g, uint8_t b
);


/**
 * Draws a rectrangle.
 * This function draws four lines, but does not fill the area within
 * the lines.
 *
 * Params:
 *   int64_t - the x coordinate of the top left corner
 *   int64_t - the y coordinate of the top left corner
 *   int64_t - the width in pixels of the rectangle
 *   int64_t - the height in pixels of the rectangle
 *   int8_t - the red component of the colour
 *   int8_t - the gren component of the colour
 *   int8_t - the blue component of the colour
 */
void k_draw_rect(
  int64_t x, int64_t y,
  int64_t w, int64_t h,
  uint8_t r, uint8_t g, uint8_t b
);


/**
 * Draws a filled rectangle.
 * The area within the four sides of the rectangle is filled with the colour
 * specified by the colour component arguments.
 *
 * Params:
 *   int64_t - the x coordinate of the top left corner
 *   int64_t - the y coordinate of the top left corner
 *   int64_t - the width in pixels of the rectangle
 *   int64_t - the height in pixels of the rectangle
 *   int8_t - the red component of the colour
 *   int8_t - the gren component of the colour
 *   int8_t - the blue component of the colour
 */
void k_fill_rect(
  int64_t x, int64_t y,
  int64_t w, int64_t h,
  uint8_t r, uint8_t g, uint8_t b
);


/**
 * Draws a triangle.
 * This function draws three lines connecting three vertices, but does not
 * fill the area within the lines.
 *
 * Params:
 *   int64_t - the x coordinate of the first vertex
 *   int64_t - the y coordinate of the first vertex
 *   int64_t - the x coordinate of the second vertex
 *   int64_t - the y coordinate of the second vertex
 *   int64_t - the x coordinate of the third vertex
 *   int64_t - the y coordinate of the third vertex
 *   int8_t - the red component of the colour
 *   int8_t - the gren component of the colour
 *   int8_t - the blue component of the colour
 */
void k_draw_triangle(
  int64_t x1, int64_t y1,
  int64_t x2, int64_t y2,
  int64_t x3, int64_t y3,
  uint8_t r, uint8_t g, uint8_t b
);


/**
 * Draws a triangle.
 * The area within the three sides of the triangle is filled with the colour
 * specified by the colour component arguments.
 *
 * Params:
 *   int64_t - the x coordinate of the first vertex
 *   int64_t - the y coordinate of the first vertex
 *   int64_t - the x coordinate of the second vertex
 *   int64_t - the y coordinate of the second vertex
 *   int64_t - the x coordinate of the third vertex
 *   int64_t - the y coordinate of the third vertex
 *   int8_t - the red component of the colour
 *   int8_t - the gren component of the colour
 *   int8_t - the blue component of the colour
 */
void k_fill_triangle(
  int64_t x1, int64_t y1,
  int64_t x2, int64_t y2,
  int64_t x3, int64_t y3,
  uint8_t r, uint8_t g, uint8_t b
);

#endif