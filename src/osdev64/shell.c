#include "osdev64/shell.h"
#include "osdev64/ps2.h"
#include "osdev64/graphics.h"
#include "osdev64/task.h"
#include "osdev64/instructor.h"
#include "osdev64/heap.h"
#include "osdev64/core.h"
#include "osdev64/file.h"
#include "osdev64/syscall.h"

#include "klibc/stdio.h"



// global system font
extern k_byte g_sys_font[4096];

// coordinates of the next character to be written
static uint64_t shell_text_x = 0; // multiple of GLYPH_WIDTH
static uint64_t shell_text_y = 0; // multiple of GLYPH_HEIGHT

// The kernel shell is assumed to be 640 pixels wide and 480 pixels high.
#define SHELL_WIDTH 640
#define SHELL_HEIGHT 480

#define OUT_BUF_SIZE 0x1000
#define VIEW_BUF_SIZE ((SHELL_HEIGHT / GLYPH_HEIGHT) * (SHELL_WIDTH / GLYPH_WIDTH))

/**
 * The output buffer contains all data that can potentially be viewed
 * on the screen. This data may exist outside the range of the current
 * view buffer
 */
static char* output_buffer;

static char* view_buffer;

static char* command_buffer;

static char* buf_writer;

static char read_buffer[IO_BUF_SIZE];

/**
 * Standard output for the shell.
 */
static FILE* shell_stdout = NULL;


static k_regn shell_write(char* str, size_t n)
{
  k_regn count = 0;
  for (size_t i = 0; i < n; i++)
  {
    char* next = buf_writer + 1;
    if (next >= output_buffer && next <= output_buffer + (OUT_BUF_SIZE - 1))
    {
      *buf_writer = str[i];
      buf_writer++;
      count++;
    }
  }

  return count;
}

static void shell_draw_glyph(
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

static void shell_putc(char c)
{
  // Limit the number of lines.
  if (shell_text_y >= CONSOLE_HEIGHT)
  {
    return;
  }

  uint8_t n = (uint8_t)c;

  if (n < 32 || n > 126)
  {
    // Handle newlines.
    if (n == 10)
    {
      shell_text_x = 0;
      shell_text_y += GLYPH_HEIGHT;
    }

    return;
  }

  // If we've reached the end of the line,
  // reset x and increment y.
  if (shell_text_x >= CONSOLE_WIDTH)
  {
    shell_text_x = 0;
    shell_text_y += GLYPH_HEIGHT;
  }

  // Locate the glyph in the font data that can be used
  // to represent the character.
  unsigned char* glyph = &(g_sys_font[(int)c * GLYPH_HEIGHT]);

  // Draw the character on the screen.
  shell_draw_glyph(glyph, shell_text_x, shell_text_y);

  // Increment x.
  if (shell_text_x < CONSOLE_WIDTH)
  {
    shell_text_x += 8;
  }
}

static void draw()
{
  shell_text_x = 0;
  shell_text_y = 0;

  int count = 0;
  // Draw the contents of the output buffer to the screen.
  for (char* reader = view_buffer; reader < buf_writer && count < VIEW_BUF_SIZE; reader++, count++)
  {
    shell_putc(*reader);
  }
}


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


static inline char decode(int sc)
{
  if (sc < 102)
  {
    return decoder_table[sc];
  }

  return '\0';
}

#define is_printable(c) (c >= 32 && c <= 126)
#define is_alpha(c) ((c >= 65 && c <= 90) || (c >= 97 && c <= 122))
#define is_num(c) (c >= 48 && c <= 57)
#define is_other(c) (shift_other(c) != '\0')

static inline char shift_num(char c)
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

static inline char shift_other(char c)
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

static void shell_action()
{
  const k_byte* key_states = k_ps2_get_key_states();
  k_ps2_event ke;

  for (;;)
  {
    int redraw = 0;
    if (k_ps2_consume_event(&ke))
    {
      char c = decode(ke.i);

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
            c = shift_num(c);
          }
          else
          {
            c = shift_other(c);
          }
        }

        // Calculate the next output buffer position to
        // determine if there is room in the buffer to write.
        // char* next = buf_writer + 1;
        // if (next >= output_buffer && next <= output_buffer + (OUT_BUF_SIZE - 1))
        // {
        //   *buf_writer = c;
        //   buf_writer++;
        //   redraw = 1;
        // }
        if (shell_write(&c, 1))
        {
          redraw = 1;
        }
      }
    }

    size_t r = k_syscall_read(shell_stdout, read_buffer, IO_BUF_SIZE);
    if (r)
    {
      if (shell_write(read_buffer, r))
      {
        redraw = 1;
      }
    }

    // redraw the shell if there was an update.
    if (redraw)
    {
      draw();
    }
  }
}


void k_shell_init()
{
  // Create the output buffer.
  output_buffer = (char*)k_heap_alloc(OUT_BUF_SIZE);
  if (output_buffer == NULL)
  {
    fprintf(stddbg, "[ERROR] failed to allocate output buffer for shell\n");
    return;
  }
  buf_writer = output_buffer;
  view_buffer = output_buffer;


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
  k_task* t = k_task_create(shell_action);
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

void* k_shell_get_stdout()
{
  return (void*)shell_stdout;
}
