#include "osdev64/shell.h"
#include "osdev64/ps2.h"
#include "osdev64/graphics.h"
#include "osdev64/task.h"
#include "osdev64/instructor.h"

#include "klibc/stdio.h"

static void draw()
{

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


static const char* lower_case = "abcdefghijklmnopqrstuvwxyz"; // 97 - 122
static const char* upper_case = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"; // 65 - 90
static const char* digits = "0123456789"; // 48 - 57

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
  default: return '\0';
  }
}

static void shell_action()
{
  const k_byte* key_states = k_ps2_get_key_states();
  k_ps2_event ke;

  for (;;)
  {
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

        fprintf(stddbg, "%c\n", c);
      }
    }
  }
}

void k_shell_init()
{
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
