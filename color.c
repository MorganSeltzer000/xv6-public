#include "types.h"
#include "user.h"

int main(int argc, char **argv)
{
  int fgcolor, bgcolor;
  if(argc>2){
    fgcolor = atoi(argv[1]);//will set to 0 if non-number
    bgcolor = atoi(argv[2]);
  } else{
    printf(1, "Correct usage is: color [fgcolor] [bgcolor]\n");
    exit();
  }
  setcolor(fgcolor, bgcolor);
  exit();
}
