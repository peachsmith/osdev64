#ifndef JEP_PS2_H
#define JEP_PS2_H

// PS/2 Interface
//
// Functions and data types for handling emulated PS/2 keyboard IRQs.


#include "osdev64/axiom.h"


// one-byte scancodes
#define PS2_SC_ESC 0x0
#define PS2_SC_1 0x1
#define PS2_SC_2 0x2
#define PS2_SC_3 0x3
#define PS2_SC_4 0x4
#define PS2_SC_5 0x5
#define PS2_SC_6 0x6
#define PS2_SC_7 0x7
#define PS2_SC_8 0x8
#define PS2_SC_9 0x9
#define PS2_SC_0 0xA
#define PS2_SC_MINUS 0xB
#define PS2_SC_EQU 0xC
#define PS2_SC_BSP 0xD
#define PS2_SC_TAB 0xE
#define PS2_SC_Q 0xF
#define PS2_SC_W 0x10
#define PS2_SC_E 0x11
#define PS2_SC_R 0x12
#define PS2_SC_T 0x13
#define PS2_SC_Y 0x14
#define PS2_SC_U 0x15
#define PS2_SC_I 0x16
#define PS2_SC_O 0x17
#define PS2_SC_P 0x18
#define PS2_SC_LSQUARE 0x19
#define PS2_SC_RSQUARE 0x1A
#define PS2_SC_ENTER 0x1B
#define PS2_SC_LCTRL 0x1C
#define PS2_SC_A 0x1D
#define PS2_SC_S 0x1E
#define PS2_SC_D 0x1F
#define PS2_SC_F 0x20
#define PS2_SC_G 0x21
#define PS2_SC_H 0x22
#define PS2_SC_J 0x23
#define PS2_SC_K 0x24
#define PS2_SC_L 0x25
#define PS2_SC_SEMI 0x26
#define PS2_SC_APOS 0x27
#define PS2_SC_TICK 0x28
#define PS2_SC_LSHIFT 0x29
#define PS2_SC_BSLASH 0x2A
#define PS2_SC_Z 0x2B
#define PS2_SC_X 0x2C
#define PS2_SC_C 0x2D
#define PS2_SC_V 0x2E
#define PS2_SC_B 0x2F
#define PS2_SC_N 0x30
#define PS2_SC_M 0x31
#define PS2_SC_COMMA 0x32
#define PS2_SC_PERIOD 0x33
#define PS2_SC_FSLASH 0x34
#define PS2_SC_RSHIFT 0x35
#define PS2_SC_KAST 0x36
#define PS2_SC_LALT 0x37
#define PS2_SC_SPACE 0x38
#define PS2_SC_CAPS 0x39
#define PS2_SC_F1 0x3A
#define PS2_SC_F2 0x3B
#define PS2_SC_F3 0x3C
#define PS2_SC_F4 0x3D
#define PS2_SC_F5 0x3E
#define PS2_SC_F6 0x3F
#define PS2_SC_F7 0x40
#define PS2_SC_F8 0x41
#define PS2_SC_F9 0x42
#define PS2_SC_F10 0x43
#define PS2_SC_NUM 0x44
#define PS2_SC_SCR 0x45
#define PS2_SC_KP_7 0x46
#define PS2_SC_KP_8 0x47
#define PS2_SC_KP_9 0x48
#define PS2_SC_KP_MINUS 0x49
#define PS2_SC_KP_4 0x4A
#define PS2_SC_KP_5 0x4B
#define PS2_SC_KP_6 0x4C
#define PS2_SC_KP_PLUS 0x4D
#define PS2_SC_KP_1 0x4E
#define PS2_SC_KP_2 0x4F
#define PS2_SC_KP_3 0x50
#define PS2_SC_KP_0 0x51
#define PS2_SC_KP_PERIOD 0x52
#define PS2_SC_PAD1 0x53
#define PS2_SC_PAD2 0x54
#define PS2_SC_PAD3 0x55
#define PS2_SC_F11 0x56
#define PS2_SC_F12 0x57

// two-byte scancodes
#define PS2_SC_RALT 0x58
#define PS2_SC_RCTRL 0x59
#define PS2_SC_LEFT 0x5A
#define PS2_SC_UP 0x5B
#define PS2_SC_RIGHT 0x5C
#define PS2_SC_DOWN 0x5D
#define PS2_SC_HOM 0x5E
#define PS2_SC_PGU 0x5F
#define PS2_SC_END 0x60
#define PS2_SC_PGD 0x61
#define PS2_SC_INS 0x62
#define PS2_SC_DEL 0x63
#define PS2_SC_KP_ENTER 0x64
#define PS2_SC_KP_FSLASH 0x65

// four-byte scancodes
#define PS2_SC_PRTSC 0x66

// six-byte scancodes
#define PS2_SC_PAUSE 0x67

#define PS2_PRESSED 1
#define PS2_RELEASED 0

typedef struct k_ps2_event {
  int i;    // index in key state array
  int type; // pressed or released
}k_ps2_event;


void k_ps2_init();


void k_ps2_handle_scancode(k_byte sc);


const k_byte* k_ps2_get_key_states();


int k_ps2_consume_event(k_ps2_event*);

const char* k_ps2_get_scstr(int);

#endif