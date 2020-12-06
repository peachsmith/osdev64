#ifndef JEP_GRAPHICS_H
#define JEP_GRAPHICS_H

#include <stdint.h>


/**
 * Initializes the graphics interface.
 */
void k_graphics_init();


/**
 * Plots a single pixel on the screen.
 * The origin (0,0) is in the top leftr corner of the screen.
 *
 * Params:
 *   uint64_t - the x coordinate
 *   uint64_t - the y coordinate
 *   int8_t - the red component of the color
 *   int8_t - the gren component of the color
 *   int8_t - the blue component of the color
 */
void k_put_pixel(uint64_t x, uint64_t y, uint8_t r, uint8_t g, uint8_t b);

/**
 * Draws a line on the screen between two points.
 *
 * Params:
 *   int64_t - the x coordinate of the first point
 *   int64_t - the y coordinate of the first point
 *   int64_t - the x coordinate of the second point
 *   int64_t - the y coordinate of the second point
 *   int8_t - the red component of the color
 *   int8_t - the gren component of the color
 *   int8_t - the blue component of the color
 */
void k_draw_line(int64_t x1, int64_t y1, int64_t x2, int64_t y2,
  uint8_t r, uint8_t g, uint8_t b);

/**
 * Initializes some form of text output.
 */
void k_text_init();

/**
 * Temporary function to test text output.
 */
void k_text_test();

#endif