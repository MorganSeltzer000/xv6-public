#include "types.h"
#include "user.h"
#include "fcntl.h"

#define FULLPAGESIZE (80*25)
#define PAGESIZE (80*23) //so can print messages on last row
#define PAGEWIDTH 80
#define LASTROW 23 //where messages are printed

char buf[FULLPAGESIZE];

void close_exit(int fd)
{
  close(fd);
  exit();
}

void readpage(int fd, int pagenum) {
  int n;
  setpos(0, 0);
  clearscr();
  if(lseek(fd, PAGESIZE * (pagenum)) < 0){
    printf(1, "editor: unable to change pos in file\n");
    close_exit(fd);
  }
  if((n = read(fd, buf, PAGESIZE)) > 0){
    if(write(1, buf, n) != n){
      printf(1, "editor: write to screen error\n");
      close_exit(fd);
    }
  } else{ //nothing to be read, make sure they know its a new page
    setpos(LASTROW, 0);
    printf(1, "New page");
  }
  setpos(0, 0);
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

  // replace newlines with spaces, to not mess up printing
  for(int i=0; i<PAGESIZE; i++){
    if(buf[i]=='\n') {
      while(i%PAGEWIDTH != PAGEWIDTH-1){
        buf[i++] = ' ';
      }
    }
    if(i%PAGEWIDTH == PAGEWIDTH-1)
      buf[i]='\n'; // still need a newline at end of lines
  }
  write(fd, buf, PAGESIZE);
}

// had the idea to create a text editor
int main(int argc, char **argv)
{
  int fd;
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

  readpage(fd, 0);

  int currpage = 0;
  uchar c;
  while(1){
    while(1){
      read(0, &c, 1); //reads in chars
      if(c == ':')
        break;
    }
    read(0, &c, 1);
    if(c=='q')
      close_exit(fd);
    else if(c=='x')
      break;
    else if(c=='n'){ //next page
      writepage(fd, currpage); //first write all the data
      readpage(fd, ++currpage);
    } else if(c=='p'){ //previous page
      writepage(fd, currpage);
      if(currpage==0){
        setpos(LASTROW, 0);
        printf(1, "Can't go below page 0");
        setpos(0, 0);
      } else{
        writepage(fd, currpage);
        readpage(fd, --currpage);
      }
    }
  }
  
  writepage(fd, currpage);
  close(fd);
  clearscr();
  setpos(0, 0);
  exit();
}
