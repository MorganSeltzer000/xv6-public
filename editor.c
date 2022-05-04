#include "types.h"
#include "user.h"
#include "fcntl.h"

#define FULLPAGESIZE (80*25)
#define PAGESIZE (80*24) //so can print messages on last row
#define PAGEWIDTH 80
#define LASTROW 24 //where messages are printed
#define MESSAGESPACE 80

char buf[FULLPAGESIZE];

void close_exit(int fd)
{
  close(fd);
  exit();
}

void writepage(int fd, int pagenum)
{
  if(lseek(fd, PAGESIZE * pagenum) < 0) {
    printf(1, "editor: unable to change pos in file\n");
    close_exit(fd);
  }
  if(getcgamem(buf, FULLPAGESIZE) < 0) {
    printf(1, "editor: unable to read cga mem\n");
    close_exit(fd);
  }
  //replace newlines with spaces, to not mess up printing
  for(int i=0; i<PAGESIZE; i++){
    if(buf[i]=='\n')
      while(i%PAGEWIDTH != PAGEWIDTH-1){
        buf[i++] = ' ';
      }
    if(i%PAGEWIDTH == PAGEWIDTH-1)
      buf[i]='\n'; //still need a newline at end of lines
  }
  write(fd, buf, PAGESIZE);
}

//had the idea to create a text editor
int main(int argc, char **argv)
{
  int fd; 
  //int editmode;//editmode is view or write?
  if(argc==1){ // no file selected
    printf(1, "editor: usage is editor [filename]\n");
    exit();
  }

  clearscr();
  fd = open(argv[1], O_CREATE | O_RDWR);
  if(fd < 0){
    printf(1, "editor: cannot open/create %s\n", argv[0]);
    exit(); // unable to create file
  }

  //do stuff
  int n;
  //allow arrows to move across row since it sees the row is filled
  setpos(0,0);
  if((n = read(fd, buf, sizeof(buf))) > 0){
    if(write(1, buf, n) != n){
      printf(1, "editor: write to screen error\n");
      exit();
    }
  }

  int currpage = 0;
  uchar c;
  while(1){
    while(1){
      read(0, &c, 1); //reads in chars
      if(c == ':') //replace with esc later
        break;
    }
    read(0, &c, 1);
    if(c=='q')
      close_exit(fd);
    else if(c=='x')
      break;
    else if(c=='n'){//next page
      //first write all the data
      writepage(fd, currpage);
      if(lseek(fd, PAGESIZE * (++currpage)) < 0){
        printf(1, "editor: unable to change pos in file\n");
        close_exit(fd);
      }
    } else if(c=='p'){//previous page
      writepage(fd, currpage);
      if(currpage==0){
        setpos(LASTROW, 0);
        printf(1, "Can't go below page 0");
      } else{
        writepage(fd, currpage);
        if(lseek(fd, PAGESIZE * (--currpage)) < 0){
          printf(1, "editor: unable to change pos in file\n");
          close_exit(fd);
        }
      }
    }
  }
  
  writepage(fd, currpage);
  close(fd);
  clearscr();
  setpos(0,0);
  exit();
}
