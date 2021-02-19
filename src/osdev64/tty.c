#include "osdev64/tty.h"
#include "osdev64/ps2.h"
#include "osdev64/graphics.h"
#include "osdev64/task.h"
#include "osdev64/instructor.h"
#include "osdev64/heap.h"
#include "osdev64/core.h"
#include "osdev64/file.h"
#include "osdev64/syscall.h"

#include "klibc/stdio.h"


// TODO: separate the TTY and shell into two separate interfaces.


// global system font
extern k_byte g_sys_font[4096];

// coordinates of the next character to be written
static uint64_t tty_cursor_x = 0; // multiple of GLYPH_WIDTH
static uint64_t tty_cursor_y = 0; // multiple of GLYPH_HEIGHT

// The kernel shell is assumed to be 640 pixels wide and 480 pixels high.
#define TTY_WIDTH 640
#define TTY_HEIGHT 480

#define OUT_BUF_SIZE 0x1000
#define CMD_BUF_SIZE 1024
#define VIEW_BUF_SIZE ((TTY_HEIGHT / GLYPH_HEIGHT) * (TTY_WIDTH / GLYPH_WIDTH))

#define is_printable(c) (c >= 32 && c <= 126)
#define is_alpha(c) ((c >= 65 && c <= 90) || (c >= 97 && c <= 122))
#define is_num(c) (c >= 48 && c <= 57)
#define is_other(c) (shift_other(c) != '\0')


/**
 * Appends a string of bytes to the output buffer.
 */
static k_regn tty_append_output(char*, size_t);

/**
 * Appends a string of bytes to the command buffer.
 */
static k_regn tty_append_command(char*, size_t);

/**
 * Removes a single byte from the command buffer.
 */
static int tty_decrement_command();

/**
 * Writes the contents of the command buffer to the standard input
 * of a process.
 */
static void tty_submit_command();

/**
 * Draws the TTY to the screen.
 */
static void tty_draw();

/**
 * Draws a symbol from font data to the screen.
 */
static void tty_draw_glyph(unsigned char*, uint64_t, uint64_t);

/**
 * Draws the cursor on the screen.
 */
static void tty_draw_cursor();

/**
 * Draws a blank space on the screen.
 */
static void tty_draw_blank();

/**
 * Draws a character to the screen.
 */
static void tty_draw_char(char);

/**
 * Converts a keyboard scancode into ASCII encoding.
 */
static inline char tty_decode(int);

/**
 * Converts a numeric character into its "shift" version.
 * This is used when the shift key is pressed.
 */
static inline char tty_shift_num(char);

/**
 * Converts a character into its "shift" version.
 * This is used when the shift key is pressed.
 */
static inline char tty_shift_other(char);


/**
 * The output buffer contains all data that can potentially be viewed
 * on the screen. This data may exist outside the range of the current
 * view buffer
 */
static char* out_buffer;
static char* out_writer;

static char* view_buffer;

static char* cmd_buffer;
static char* cmd_writer;

// A buffer for reading from output streams.
static char read_buffer[IO_BUF_SIZE];


/**
 * Standard output for the shell.
 */
static FILE* shell_stdout = NULL;

// Used for decoding PS2 scancodes.
static const char decoder_table[102] = {
  27,  49,  50,  51,  52,  53,  54,  55,  56,  57,  // escape 1 - 9
  48,  45,  61,  8,   9,   113, 119, 101, 114, 116, // 0 - = backspace, horizontal tab QWERT
  121, 117, 105, 111, 112, 91,  93,  10,  0,   97,  // YUIOP[] enter ctrl A
  115, 100, 102, 103, 104, 106, 107, 108, 59,  39,  // SDFGHJKL;'
  96,  0,   92,  122, 120, 99,  118, 98,  110, 109, // ` shift '\' ZXCVBNM
  44,  46,  47,  0,   42,  0,   32,  0,   0,   0,   // , . / shift * alt space caps F1 F2
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   // F3 F4 F5 F6 F7 F8 F9 F10 num lock scroll lock
  55,  56,  57,  45,  52,  53,  54,  43,  49,  50,  // (keypad) 7 8 9 - 4 5 6 + 1 2 3 
  51,  48,  46,  0,   0,   0,   0,   0,   0,   0,   // (keypad) 0 . pad pad pad F11 F12 alt ctrl 
  0,   0,   0,   0,   0,   0,   0,   0,   0,   127, // ctrl left up right down home pageup end pagedown insert delete
  10,  47                                           // (keypad) enter /
};


static k_regn tty_append_output(char* str, size_t n)
{
  k_regn count = 0;
  for (size_t i = 0; i < n; i++)
  {
    char* next = out_writer + 1;
    if (next >= out_buffer && next <= out_buffer + (OUT_BUF_SIZE - 1))
    {
      *out_writer = str[i];
      out_writer++;
      count++;
    }
  }

  // Determine if we need to scroll the view offset.
  char* view_start = view_buffer;
  char* next_view_start = NULL;
  int lines = 0;
  int line_chars = 0;
  for (char* reader = view_buffer; reader < out_writer; reader++)
  {
    if (*reader != '\n')
    {
      line_chars++;
      if (line_chars == TTY_WIDTH / GLYPH_WIDTH)
      {
        line_chars = 0;
        lines++;
        if (lines == 2)
        {
          view_start = reader;
        }
      }
    }
    else
    {
      line_chars = 0;
      lines++;
      if (lines == 2)
      {
        view_start = reader;
      }
    }
  }

  // If the output buffer exceeds the view window,
  // update the view offset.
  if (lines >= TTY_HEIGHT / GLYPH_HEIGHT)
  {
    view_buffer = view_start;
  }

  return count;
}

static k_regn tty_append_command(char* str, size_t n)
{
  k_regn count = 0;
  for (size_t i = 0; i < n; i++)
  {
    char* next = cmd_writer + 1;
    if (next >= cmd_buffer && next <= cmd_buffer + (CMD_BUF_SIZE - 1))
    {
      *cmd_writer = str[i];
      cmd_writer++;
      count++;
    }
  }

  return count;
}

static int tty_decrement_command()
{
  if (cmd_writer > cmd_buffer)
  {
    cmd_writer--;
    if (out_writer > out_buffer)
    {
      out_writer--;
    }
    tty_draw_blank();
    return 1;
  }

  return 0;
}

static void tty_submit_command()
{
  // Clear the previous cursor.
  tty_draw_blank();

  // Add a newline to the output buffer.
  tty_append_output("\n", 1);

  // Reset the command buffer.
  cmd_writer = cmd_buffer;
}

static void tty_draw_glyph(
  unsigned char* glyph,
  uint64_t x,
  uint64_t y
)
{
  // Plot a pixel for each bit with a value of 1.
  for (int i = 0; i < GLYPH_HEIGHT; i++)
  {
    for (int j = 0; j < GLYPH_WIDTH; j++)
    {
      if ((glyph[i] >> (7 - j)) & 1)
      {
        k_put_pixel(x + j, y + i, 220, 220, 220);
      }
      else
      {
        k_put_pixel(x + j, y + i, 0, 0, 0);
      }
    }
  }
}

static void tty_draw_blank()
{
  // If we've reached the end of the line, then return.
  // Blanks aren't used to fill multiple lines.
  if (tty_cursor_x >= TTY_WIDTH)
  {
    return;
  }

  // Draw a solid block of background color.
  for (int i = 0; i < GLYPH_HEIGHT; i++)
  {
    for (int j = 0; j < GLYPH_WIDTH; j++)
    {
      k_put_pixel(tty_cursor_x + j, tty_cursor_y + i, 0, 0, 0);
    }
  }
}

static void tty_draw_cursor()
{
  // If we've reached the end of the line,
  // reset x and increment y.
  if (tty_cursor_x >= TTY_WIDTH)
  {
    tty_cursor_x = 0;
    tty_cursor_y += GLYPH_HEIGHT;
  }

  // Draw a solid block of foreground color.
  for (int i = 0; i < GLYPH_HEIGHT; i++)
  {
    for (int j = 0; j < GLYPH_WIDTH; j++)
    {
      k_put_pixel(tty_cursor_x + j, tty_cursor_y + i, 220, 220, 220);
    }
  }
}

static void tty_draw_char(char c)
{
  // Limit the number of lines.
  if (tty_cursor_y >= TTY_HEIGHT)
  {
    return;
  }

  if (!is_printable(c))
  {
    // Handle newlines.
    if (c == '\n')
    {
      for (;tty_cursor_x < TTY_WIDTH;)
      {
        tty_draw_char(' ');
      }
      tty_cursor_x = 0;
      tty_cursor_y += GLYPH_HEIGHT;
    }

    return;
  }

  // If we've reached the end of the line,
  // reset x and increment y.
  if (tty_cursor_x >= TTY_WIDTH)
  {
    tty_cursor_x = 0;
    tty_cursor_y += GLYPH_HEIGHT;
  }

  // Locate the glyph in the font data that can be used
  // to represent the character.
  unsigned char* glyph = &(g_sys_font[(int)c * GLYPH_HEIGHT]);

  // Draw the character on the screen.
  tty_draw_glyph(glyph, tty_cursor_x, tty_cursor_y);

  // Increment x.
  if (tty_cursor_x < TTY_WIDTH)
  {
    tty_cursor_x += GLYPH_WIDTH;
  }
}

static void tty_draw()
{
  tty_cursor_x = 0;
  tty_cursor_y = 0;

  int count = 0;
  int drawn_cursor = 0;

  // Draw the contents of the output buffer to the screen.
  for (char* reader = view_buffer; reader < out_writer && count < VIEW_BUF_SIZE; reader++, count++)
  {
    tty_draw_char(*reader);
  }

  // Draw the cursor after everything else.
  if (count++ < VIEW_BUF_SIZE)
  {
    tty_draw_cursor();
  }

  uint64_t cx = tty_cursor_x;

  // UNDER CONSTRUCTION:
  // Clear the rest of the current line after the cursor.
  tty_cursor_x += GLYPH_WIDTH;
  for (;tty_cursor_x < TTY_WIDTH;)
  {
    // tty_draw_char(' ');
    tty_draw_blank();
    tty_cursor_x += GLYPH_WIDTH;
  }

  tty_cursor_x = cx;

  // for (tty_cursor_x += GLYPH_WIDTH;tty_cursor_x < TTY_WIDTH;)
  // {
  //   tty_draw_char(' ');
  // }
}

static inline char tty_decode(int sc)
{
  if (sc < 102)
  {
    return decoder_table[sc];
  }

  return '\0';
}

static inline char tty_shift_num(char c)
{
  switch (c)
  {
  case '1': return '!';
  case '2': return '@';
  case '3': return '#';
  case '4': return '$';
  case '5': return '%';
  case '6': return '^';
  case '7': return '&';
  case '8': return '*';
  case '9': return '(';
  case '0': return ')';
  default: return '\0';
  }
}

static inline char tty_shift_other(char c)
{
  switch (c)
  {
  case '-': return '_';
  case '=': return '+';
  case '[': return '{';
  case ']': return '}';
  case ';': return ':';
  case '\'': return '"';
  case ',': return '<';
  case '.': return '>';
  case '/': return '?';
  case '\\': return '|';
  case '`': return '~';
  case ' ': return ' ';
  default: return '\0';
  }
}

static void tty_action()
{
  const k_byte* key_states = k_ps2_get_key_states();
  k_ps2_event ke;

  tty_draw();

  for (;;)
  {
    int redraw = 0;
    if (k_ps2_consume_event(&ke))
    {
      char c = tty_decode(ke.i);

      if (ke.type == PS2_PRESSED && is_printable(c))
      {
        if (key_states[PS2_SC_LSHIFT] || key_states[PS2_SC_RSHIFT])
        {
          if (is_alpha(c))
          {
            c -= 32;
          }
          else if (is_num(c))
          {
            c = tty_shift_num(c);
          }
          else
          {
            c = tty_shift_other(c);
          }
        }

        if (tty_append_command(&c, 1))
        {
          tty_append_output(&c, 1);
          redraw = 1;
        }
      }
      else if (ke.type == PS2_PRESSED)
      {
        switch (ke.i)
        {
        case PS2_SC_ENTER:
        case PS2_SC_KP_ENTER:
          tty_submit_command();
          redraw = 1;
          break;

        case PS2_SC_BSP:
        {
          if (tty_decrement_command())
          {
            redraw = 1;
          }
        }
        break;

        default:
          break;
        }
      }
    }

    // Read the standard output from the shell.
    size_t r = k_syscall_read(shell_stdout, read_buffer, IO_BUF_SIZE);
    if (r)
    {
      if (tty_append_output(read_buffer, r))
      {
        redraw = 1;
      }
    }

    // redraw the terminal if there was an update.
    if (redraw)
    {
      tty_draw();
    }
  }
}


void k_tty_init()
{
  // Create the output buffer.
  out_buffer = (char*)k_heap_alloc(OUT_BUF_SIZE);
  if (out_buffer == NULL)
  {
    fprintf(stddbg, "[ERROR] failed to allocate output buffer for shell\n");
    return;
  }
  out_writer = out_buffer;
  view_buffer = out_buffer;

  // Create the command buffer.
  cmd_buffer = (char*)k_heap_alloc(CMD_BUF_SIZE);
  if (cmd_buffer == NULL)
  {
    fprintf(stddbg, "[ERROR] failed to allocate command buffer for shell\n");
    return;
  }
  cmd_writer = cmd_buffer;


  // Create a file object for standard output.
  shell_stdout = (FILE*)k_heap_alloc(sizeof(FILE));
  if (shell_stdout == NULL)
  {
    fprintf(stddbg, "[ERROR] failed to allocate memory for shell stdout\n");
    return;
  }

  k_finfo* info = k_file_create_info(__FILE_NO_STDOUT);
  if (info == NULL)
  {
    fprintf(stddbg, "[ERROR] failed to allocate memory for shell info\n");
    return;
  }

  info->type = __FILE_NO_STDOUT;
  info->writer = info->buf;
  info->reader = info->buf;

  shell_stdout->info = info;

  // Start the shell task.
  k_task* t = k_task_create(tty_action);
  if (t == NULL)
  {
    fprintf(
      stddbg,
      "[ERROR] failed to create kernel shell task\n"
    );
    HANG();
  }
  k_task_schedule(t);
}

void* k_tty_get_shell_stdout()
{
  return (void*)shell_stdout;
}
