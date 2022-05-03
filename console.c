// Console input and output.
// Input is from the keyboard or serial port.
// Output is written to the screen and serial port.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

static void consputc(int);

static int panicked = 0;
static int colormask = 0x0700;

static struct {
  struct spinlock lock;
  int locking;
} cons;

static void
printint(int xx, int base, int sign)
{
  static char digits[] = "0123456789abcdef";
  char buf[16];
  int i;
  uint x;

  if(sign && (sign = xx < 0))
    x = -xx;
  else
    x = xx;

  i = 0;
  do{
    buf[i++] = digits[x % base];
  }while((x /= base) != 0);

  if(sign)
    buf[i++] = '-';

  while(--i >= 0)
    consputc(buf[i]);
}
//PAGEBREAK: 50

// Print to the console. only understands %d, %x, %p, %s.
void
cprintf(char *fmt, ...)
{
  int i, c, locking;
  uint *argp;
  char *s;

  locking = cons.locking;
  if(locking)
    acquire(&cons.lock);

  if (fmt == 0)
    panic("null fmt");

  argp = (uint*)(void*)(&fmt + 1);
  for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
    if(c != '%'){
      consputc(c);
      continue;
    }
    c = fmt[++i] & 0xff;
    if(c == 0)
      break;
    switch(c){
    case 'd':
      printint(*argp++, 10, 1);
      break;
    case 'x':
    case 'p':
      printint(*argp++, 16, 0);
      break;
    case 's':
      if((s = (char*)*argp++) == 0)
        s = "(null)";
      for(; *s; s++)
        consputc(*s);
      break;
    case '%':
      consputc('%');
      break;
    default:
      // Print unknown % sequence to draw attention.
      consputc('%');
      consputc(c);
      break;
    }
  }

  if(locking)
    release(&cons.lock);
}

void
panic(char *s)
{
  int i;
  uint pcs[10];

  cli();
  cons.locking = 0;
  // use lapiccpunum so that we can call panic from mycpu()
  cprintf("lapicid %d: panic: ", lapicid());
  cprintf(s);
  cprintf("\n");
  getcallerpcs(&s, pcs);
  for(i=0; i<10; i++)
    cprintf(" %p", pcs[i]);
  panicked = 1; // freeze other CPU
  for(;;)
    ;
}

//PAGEBREAK: 50
#define BACKSPACE 0x101 //not 0x100 because if ansi escape undefined its 0x100
#define KEY_UP 0x1E2
#define KEY_DN 0x1E3
#define KEY_LF 0x1E4
#define KEY_RT 0x1E5
#define CRTPORT 0x3d4

#define getcgapos(pos) outb(CRTPORT, 14);\
  pos = inb(CRTPORT+1) << 8;\
  outb(CRTPORT, 15);\
  pos |= inb(CRTPORT+1);
#define putcgapos(pos) outb(CRTPORT, 14);\
  outb(CRTPORT+1, (pos)>>8);\
  outb(CRTPORT, 15);\
  outb(CRTPORT+1, (pos));
#define checkcgabounds(pos) if(pos < 0 || pos > 25*80)\
    panic("pos under/overflow");
static ushort *crt = (ushort*)P2V(0xb8000);  // CGA memory

static void
cgaputc(int c)
{
  int pos;

  // Cursor position: col + 80*row.
  outb(CRTPORT, 14);
  pos = inb(CRTPORT+1) << 8;
  outb(CRTPORT, 15);
  pos |= inb(CRTPORT+1);

  if(c == '\n')
    pos += 80 - pos%80;
  else if(c == KEY_DN)
    pos += 80;
  else if(c == KEY_UP){
    if(pos >= 80) pos -= 80;
  } else if(c==BACKSPACE || c==KEY_LF){
    if(pos > 0) --pos;
  } else
    crt[pos++] = (c&0xff) | colormask;

  if(pos < 0 || pos > 25*80)
    panic("pos under/overflow");

  if((pos/80) >= 24){  // Scroll up.
    memmove(crt, crt+80, sizeof(crt[0])*23*80);
    pos -= 80;
    memset(crt+pos, 0, sizeof(crt[0])*(24*80 - pos));
  }

  outb(CRTPORT, 14);
  outb(CRTPORT+1, pos>>8);
  outb(CRTPORT, 15);
  outb(CRTPORT+1, pos);
  if(!(c & 0x100 && c != BACKSPACE)) //if its not KEY_LF or KEY_RT
    crt[pos] = ' ' | colormask;
}

void
consputc(int c)
{
  if(panicked){
    cli();
    for(;;)
      ;
  }
  if(c & 0x100){
    if(c == BACKSPACE){
      uartputc('\b'); uartputc(' '); uartputc('\b');
    } else if(c == KEY_LF)
      uartputc('\b');
    else
      uartputc(c & 0xFF);
  } else
    uartputc(c);
  cgaputc(c);
}

#define uarthundred(x) if(x>99) {\
  uartputc('0' + (x/100) % 10);\
  uartputc('0' + (x/10) % 10);\
  } else if(x>9)\
  uartputc('0' + (x/10) % 10);\
  uartputc('0' + x % 10);

void
consmoveup(int amount)
{
  uartputc(0x1b);
  uartputc(0x5b);
  uarthundred(amount);
  uartputc('A');
  while(amount-- > 0)
    cgaputc(KEY_UP); //can just use this since doesnt scroll
}

//note this does not scroll the screen down
void
consmovedown(int amount)
{
  int pos;
  uartputc(0x1b);
  uartputc(0x5b);
  uarthundred(amount);
  uartputc('B');
  getcgapos(pos);
  if(pos+amount*80 < 80*24)
    pos += amount*80;
  else
    pos = 80*24 + pos%80;
  checkcgabounds(pos);
  putcgapos(pos);
}

// does not change cursor position
void
consclearscreen()
{
  uartputc(0x1b);
  uartputc(0x5b);
  uartputc('0'+2);
  uartputc('J');
  //todo for cga, set everything in array to 0
}

void
consmovepos(uint y, uint x)
{
  if(y>24)
	  y=24;
  x=x%80; //this is done for cga, uart doesn't care
  uartputc(0x1b);
  uartputc(0x5b);
  uarthundred(y);
  uartputc(';');
  uarthundred(x);
  uartputc('H');
  putcgapos(y*80+x);
}

void
consolesetcolor(uint fgcolor, uint bgcolor) {
  if(fgcolor>7 || bgcolor>7) // color is too high
    return; // don't set anything
  uartputc(0x1b); // set colors for UART
  uartputc(0x5b);
  uartputc('3'); // 30 is start of fg colors
  uartputc('0' + fgcolor);
  uartputc(';');
  uartputc('4'); // 40 is start of bg colors
  uartputc('0' + bgcolor);
  uartputc('m');
  colormask = fgcolor << 8 | bgcolor << 12;
}

#define INPUT_BUF 128
struct {
  char buf[INPUT_BUF];
  uint r;  // Read index
  uint w;  // Write index
  uint e;  // Edit index (furthest char edited)
  uint p;  // Cursor offset (how far left from e)
} input;

#define C(x)  ((x)-'@')  // Control-x

void
consoleintr(int (*getc)(void))
{
  int c, doprocdump = 0, tmp_p = 0;

  acquire(&cons.lock);
  while((c = getc()) >= 0){
    switch(c){
    case C('P'):  // Process listing.
      // procdump() locks cons.lock indirectly; invoke later
      doprocdump = 1;
      break;
    case C('U'):  // Kill line.
      while(input.p > 0){
        input.p--;
        consputc(' ');
      }
      while(input.e != input.w &&
            input.buf[(input.e-1) % INPUT_BUF] != '\n'){
        input.e--;
        consputc(BACKSPACE);
      }
      break;
    case C('H'): case '\x7f':  // Backspace
      if(input.e == input.w || input.e - input.p == input.w)
        break;
      if(input.p == 0)
        consputc(BACKSPACE);
      else{
        tmp_p = input.p;
        consputc(KEY_LF);
        while(input.p > 0){ // have to shift chars over by 1
          consputc(input.buf[input.e - input.p - 1]=input.buf[input.e - input.p]);
          input.p--;
        }
        consputc(' '); //overwrite last char
        input.p = tmp_p;
        while(tmp_p-- >= 0){ // since going back before the space is p0
          consputc(KEY_LF); //moves back to starting pos
        }
      }
      input.e--;
      break;
    case KEY_LF:
      if(input.e-input.p != input.w){
        input.p++;
        consputc(KEY_LF);
      }
      break;
    case KEY_RT:
      if(input.p>0){
        // rewrite previous char
        consputc(0x100 | (input.buf[(input.e - --input.p - 1) % INPUT_BUF]));
      }
      break;
    default: //current
      if(c != 0 && input.e - input.p - input.r < INPUT_BUF){
        c = (c == '\r') ? '\n' : c;
        if(c == '\n' || c == C('D') || input.e == input.r+INPUT_BUF){
          input.buf[input.e++ % INPUT_BUF] = c; //not offset by p, since newline
          input.w = input.e;
          input.p = 0;
          consputc(c);
          wakeup(&input.r);
        } else {
          input.buf[(input.e - input.p) % INPUT_BUF] = c;
          if(input.p > 0){
            consputc(c | 0x100);
            input.p--;
          } else{
            input.e++;
            consputc(c);
          }
        }
      }
      break;
    }
  }
  release(&cons.lock);
  if(doprocdump) {
    procdump();  // now call procdump() wo. cons.lock held
  }
}

int
consoleread(struct inode *ip, char *dst, int n)
{
  uint target;
  int c;

  iunlock(ip);
  target = n;
  acquire(&cons.lock);
  while(n > 0){
    while(input.r == input.w){
      if(myproc()->killed){
        release(&cons.lock);
        ilock(ip);
        return -1;
      }
      sleep(&input.r, &cons.lock);
    }
    c = input.buf[input.r++ % INPUT_BUF];
    if(c == C('D')){  // EOF
      if(n < target){
        // Save ^D for next time, to make sure
        // caller gets a 0-byte result.
        input.r--;
      }
      break;
    }
    *dst++ = c;
    --n;
    if(c == '\n')
      break;
  }
  release(&cons.lock);
  ilock(ip);

  return target - n;
}

int
consolewrite(struct inode *ip, char *buf, int n)
{
  int i;

  iunlock(ip);
  acquire(&cons.lock);
  for(i = 0; i < n; i++)
    consputc(buf[i] & 0xff);
  release(&cons.lock);
  ilock(ip);

  return n;
}

void
consoleinit(void)
{
  initlock(&cons.lock, "console");

  devsw[CONSOLE].write = consolewrite;
  devsw[CONSOLE].read = consoleread;
  cons.locking = 1;

  ioapicenable(IRQ_KBD, 0);
}

