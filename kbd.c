#include "types.h"
#include "x86.h"
#include "defs.h"
#include "kbd.h"

int
kbdgetc(void)
{
  static uint shift;
  static uchar *charcode[4] = {
    normalmap, shiftmap, ctlmap, ctlmap
  };
  //stores nums for altcode, max 3 in altcode but 3rd not stored
  static uchar altbuf[2]; 
  static uchar altbufpos = 0;
  uint st, data, c;

  st = inb(KBSTATP);
  if((st & KBS_DIB) == 0)
    return -1;
  data = inb(KBDATAP);

  if(data == 0xE0){
    shift |= E0ESC;
    return 0;
  } else if(data & 0x80){
    // Key released
    data = (shift & E0ESC ? data : data & 0x7F);
    shift &= ~(shiftcode[data] | E0ESC);
    if(shiftcode[data] == ALT && altbufpos > 0 ){ // alt code stuff
      if(altbufpos==1) {
        altbufpos = 0;
        return altbuf[0]-'0';
      } else { // altbufpos should be 2, if its larger somehow doesn't matter
        altbufpos = 0;
        return (altbuf[0]-'0')*10 + altbuf[1]-'0';
      }
    }
    return 0;
  } else if(shift & E0ESC){
    // Last character was an E0 escape; or with 0x80
    data |= 0x80;
    shift &= ~E0ESC;
  }

  shift |= shiftcode[data];
  shift ^= togglecode[data];
  c = charcode[shift & (CTL | SHIFT)][data];
  if (shift & ALT){
    if(c >= '0' && c <= '9') {
      if(altbufpos>1){
        altbufpos = 0;
        return ((altbuf[0]-'0')*100 + (altbuf[1]-'0')*10 + (c-'0')) % 255;
      }
      altbuf[altbufpos++] = c;
      return 0;
    }
    return c;
  } else if ((c & 0xE0) == 0xE0){
    c |= SPECIALKEY; //so not confused with uart putting legitimate 0xE# chars
    return c;
  } else if(shift & CAPSLOCK){
    if('a' <= c && c <= 'z')
      c += 'A' - 'a';
    else if('A' <= c && c <= 'Z')
      c += 'a' - 'A';
  }
  return c;
}

void
kbdintr(void)
{
  consoleintr(kbdgetc);
}
