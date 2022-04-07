// Constants needed for UART

#define ESCAPEMAPSTART 0x40
#define SPECIALKEY 0x100 // 'or'ed with special keys not in ascii
// Special keycodes, same as kbd.h
#define KEY_UP     0xE2
#define KEY_DN     0xE3
#define KEY_LF     0xE4 // KEY_RT occurs before LF in ANSI, keeping HID order
#define KEY_RT     0xE5
#define KEY_PGUP   0xE6
#define KEY_PGDN   0xE7

static uchar escapemap[64] = // special keys only defined 0x40-0x7F
{
  0, KEY_UP, KEY_DN, KEY_RT, KEY_LF, 0, 0, 0, //0x40
  0, 0, 0, 0, 0, 0, 0, 0, // 0x48
  0, 0, 0, 0, 0, 0, 0, 0, // 0x50
  0, 0, 0, 0, 0, 0, 0, 0, // 0x58
  0, 0, 0, 0, 0, 0, 0, 0, // 0x60
  0, 0, 0, 0, 0, 0, 0, 0, // 0x68
  0, 0, 0, 0, 0, 0, 0, 0, // 0x70
  0, 0, 0, 0, 0, 0, 0, 0 // 0x78
};
