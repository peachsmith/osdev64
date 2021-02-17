#include "osdev64/ps2.h"
#include "osdev64/memory.h"
#include "osdev64/heap.h"

#include "klibc/stdio.h"


#define EVENT_BUF_SIZE 1024

static volatile k_byte* key_states;

static volatile k_byte sc_count = 0;
static volatile k_byte sc_seq[6];
static volatile k_byte prev_key = 0;

// circular buffer for PS2 key events
static volatile k_ps2_event* event_buffer;

static volatile k_ps2_event* buf_reader;
static volatile k_ps2_event* buf_writer;

// scancode set 1
// 100 unique scancode sequences, with 3 padding codes
// The pause key is not included in this list, since there is
// no "key release" event for that key.
const char* sc1_map[103] = {

  // one-byte sequences
  "ESC", "1",   "2",  "3",   "4",   "5",   "6",   "7",   "8",   "9",   // 9
  "0",   "-",   "=",  "BSP", "TAB", "Q",   "W",   "E",   "R",   "T",   // 19
  "Y",   "U",   "I",  "O",   "P",   "[",   "]",   "ENT", "LCT", "A",   // 29
  "S",   "D",   "F",  "G",   "H",   "J",   "K",   "L",   ";",   "'",   // 39
  "`",   "LSH", "\\", "Z",   "X",   "C",   "V",   "B",   "N",   "M",   // 49
  ",",   ".",   "/",  "RSH", "K*",  "LAT", "SPC", "CAP", "F1",  "F2",  // 59
  "F3",  "F4",  "F5", "F6",  "F7",  "F8",  "F9",  "F10", "NUM", "SCL", // 69
  "K7",  "K8",  "K9", "K-",  "K4",  "K5",  "K6",  "K+",  "K1",  "K2",  // 79
  "K3",  "K0",  "K.", "",    "",    "",    "F11", "F12",               // 87

  // two-byte sequences
  "RAT", "RCT", "LA", "UA", "RA", "DA", // 93
  "HOM", "PGU", "END", "PGD", "INS", "DEL", // 99
  "KEN", "K/", // 101

  // four-byte sequences
  "PRT" // 102
};


static int two_byte_i(k_byte b);
static inline void handle_scancode(k_byte sc);
static void write_event(k_ps2_event*);
static int read_event(k_ps2_event*);

void k_ps2_init()
{
  key_states = (volatile k_byte*)k_heap_alloc(103);
  if (key_states == NULL)
  {
    fprintf(
      stddbg,
      "[ERROR] failed to allocate memory for key states\n"
    );
    HANG();
  }

  for (int i = 0; i < 103; i++)
  {
    key_states[i] = 0;
  }

  event_buffer = (volatile k_ps2_event*)k_heap_alloc(sizeof(k_ps2_event) * EVENT_BUF_SIZE);
  if (event_buffer == NULL)
  {
    fprintf(
      stddbg,
      "[ERROR] failed to allocate memory for key event buffer\n"
    );
    HANG();
  }

  for (int i = 0; i < EVENT_BUF_SIZE; i++)
  {
    event_buffer[i].i = 0;
    event_buffer[i].type = 0;
  }

  buf_reader = event_buffer;
  buf_writer = event_buffer;
}

static int two_byte_i(k_byte b)
{
  switch (b & 0x7F)
  {
  case 0x4B: // left arrow
    return 90;

  case 0x48: // up arrow
    return 91;

  case 0x4D: // right arrow
    return 92;

  case 0x50: // down arrow
    return 93;

  case 0x38: // right alt
    return 88;

  case 0x1D: // right control
    return 89;

  case 0x47: // home
    return 94;

  case 0x49: // page up
    return 95;

  case 0x4F: // end
    return 96;

  case 0x51: // page down
    return 97;

  case 0x52: // insert
    return 98;

  case 0x53: // delete
    return 99;

  case 0x1C: // keypad enter
    return 100;

  case 0x35: // keypad /
    return 101;

  default:
    return 0;
  }
}

static void write_event(k_ps2_event* e)
{
  // Calculate the next position of the writer pointer.
  // If the writer pointer has reached the end of the buffer,
  // reset it to the beginning.
  volatile k_ps2_event* next = buf_writer + 1;
  if (next >= event_buffer + (EVENT_BUF_SIZE - 1))
  {
    next = event_buffer;
  }

  // If the writer pointer is about to overtake the reader pointer,
  // don't write any events.
  if (next == buf_reader)
  {
    return;
  }

  // Write the event to the buffer and advance the writer pointer.
  *buf_writer = *e;
  buf_writer = next;
}

static int read_event(k_ps2_event* e)
{
  // TODO: add some sort of synchronization for reading
  // key events. Only the process with "focus" should
  // be able to advance the reader pointer.

  // If the reader pointer has overtaken the writer pointer,
  // don't read any mroe events.
  if (buf_reader == buf_writer)
  {
    return 0;
  }

  // Calculate the next position of the reader pointer.
  // If the reader pointer has reached the end of the buffer,
  // reset it to the beginning.
  volatile k_ps2_event* next = buf_reader + 1;
  if (next >= event_buffer + (EVENT_BUF_SIZE - 1))
  {
    next = event_buffer;
  }

  // Read an event from the buffer and advance the reader pointer.
  *e = *buf_reader;
  buf_reader = next;

  return 1;
}

void k_ps2_handle_scancode(k_byte sc)
{
  k_ps2_event ke = { 0, 0 };

  if (sc_count > 0)
  {
    sc_seq[sc_count++] = sc;

    if (sc_count == 2 && sc_seq[0] == 0xE0)
    {
      if (sc_seq[1] != 0xB7 && sc_seq[1] != 0x2A)
      {
        // Two-byte scancode
        // fprintf(
        //   stddbg,
        //   "key %s %s\n",
        //   sc1_map[two_byte_i(sc_seq[1])],
        //   (sc_seq[1] < 0x90) ? "pressed" : "released"
        // );
        ke.i = two_byte_i(sc_seq[1]);
        ke.type = (sc_seq[1] < 0x90) ? PS2_PRESSED : PS2_RELEASED;
        // key_states[two_byte_i(sc_seq[1])] = (sc_seq[1] < 0x90) ? 1 : 0;
        key_states[ke.i] = ke.type;

        sc_count = 0;

        // key_event = ke;
        write_event(&ke);
      }
    }
    else if (sc_count == 4)
    {
      // Four-byte scancode (print screen key)
      if (sc_seq[3] == 0x37 || sc_seq[3] == 0xAA)
      {
        // fprintf(stddbg, "print screen %s\n", (sc_seq[3] == 0x37) ? "pressed" : "released");
        ke.i = PS2_SC_PRTSC;
        ke.type = (sc_seq[3] == 0x37) ? PS2_PRESSED : PS2_RELEASED;
        // key_states[PS2_SC_PRTSC] = (sc_seq[3] == 0x37) ? 1 : 0;
        key_states[PS2_SC_PRTSC] = ke.type;

        sc_count = 0;

        // key_event = ke;
        write_event(&ke);
      }
    }
    else if (sc_count >= 6)
    {
      // Size-byte scancode (pause key)
      // fprintf(stddbg, "pause pressed\n");
      sc_count = 0;
    }
  }
  else if (sc == 0xE0 || sc == 0xE1)
  {
    // Begin a scancode sequence.
    prev_key = sc;
    sc_seq[sc_count++] = sc;
  }
  else if ((sc & 0x7F) < 89)
  {
    // Single-byte scancode
    // fprintf(
    //   stddbg,
    //   "key %s %s\n",
    //   sc1_map[(sc & 0x7F) - 1],
    //   (sc & 0x80) ? "released" : "pressed"
    // );
    ke.i = (sc & 0x7F) - 1;
    ke.type = (sc & 0x80) ? PS2_RELEASED : PS2_PRESSED;
    // key_states[(sc & 0x7F) - 1] = (sc & 0x80) ? 0 : 1;
    key_states[ke.i] = ke.type;

    // key_event = ke;
    write_event(&ke);
  }
  else
  {
    // Unknown key
    // fprintf(
    //   stddbg,
    //   "unknown key %X\n",
    //   sc
    // );
  }
}

const k_byte* k_ps2_get_key_states()
{
  return (const k_byte*)key_states;
}

int k_ps2_consume_event(k_ps2_event* e)
{
  return read_event(e);
}

const char* k_ps2_get_scstr(int sc)
{
  if (sc < 103)
  {
    return sc1_map[sc];
  }

  // If the sc was an unexpected value,
  // return one of the padding strings.
  return sc1_map[85];
}
