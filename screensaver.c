#include "types.h"
#include "user.h"

// a screensaver application
int main()
{
  uint y = 0;
  uint x = 1;
  clearscr();
  setpos(y, x);
  while(1){
    y++;
    x++;
    if(y>24){
      y=0;
      x+=1;
    } else if(x>=80)
      x=0;
    setcolor(rand() % 8, rand() % 8);
    setpos(y, x);
    printf(1, "*");
    sleep(1);
  }
}
