// command handler
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#define MAXWORDS 50
#define MAXBUFFER 5120
#define MAXLINES 100
enum { OFF = 0, ON };

extern int terminalnl, linesdisplayed;

// function declarations
size_t readfileentry(int fd, char *line);
unsigned int isseparationchar(char t);
unsigned int isfdopen(int fd);
int chartofd(char *line, char *filename);
int assignstringvaluestoarray(char *line, char array[MAXWORDS][MAXBUFFER/2], int entries);
int readstringentry(char *line, char *linepart);
char* limitspaces(char *tstring);
int stringformattersplitter(char *text, char bufferinlines[MAXLINES][MAXBUFFER/2]);
int isprintablecharacter(int t);

// read entry from file
size_t readfileentry(int fd, char *line)
{
  char buf[1];
  int i=0;
  size_t nread;
    
    while ((nread=read(fd, buf, sizeof(buf)))) {
     if (isseparationchar(buf[0])) {
      if (i==0) // no characters in line, separation character skipped
       continue;
      break; // break read, separation character not recorded
     }
     line[i++]=buf[0];
    }
    line[i]='\0';
  
    // file ended, close file descriptor
    if (nread==0)
     close(fd);
    
 return nread;
}

enum { NONE=0, WORD };
// word separation characters
unsigned int isseparationchar(char t)
{
  if (t==' ' || t=='\n' || t=='\r')
   return WORD;

 return NONE;
}

// if fd open
unsigned int isfdopen(int fd)
{
  struct stat tfd;
  
   if (fstat(fd, &tfd) == -1)
    return 0;
   
 return 1;
}

// char array to file descriptor
int chartofd(char *line, char *filename)
{
  int fd, nwrite;
  mkstemp(filename);
  
   fd=open(filename, O_WRONLY| O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
   if ( (nwrite=write(fd, line, (size_t) strlen(line))==-1) )
    return 0;
   close(fd);
   fd=open(filename, O_RDONLY);
   
 return fd;
}

// using readstringentry, assign strings to array of pointers
int assignstringvaluestoarray(char *line, char array[MAXWORDS][MAXBUFFER/2], int entries)
{
  int actualentries=0;
  char tline[MAXBUFFER];

   while ((readstringentry(line, tline)) && actualentries<entries)
    strcpy(array[actualentries++], tline);

 return actualentries;
}

// read entry from file
int readstringentry(char *line, char *linepart)
{
  static int i=0; // source line position
  int i1=0; // word position
  static char tline[MAXBUFFER];

  // reset static
  if (i==strlen(line)) {
   i=0;
   return i;
  }
  if ((i && strcmp(line, tline)) || i==0) {
   i=0;
   strcpy(tline, line);
  }

    while (line[i]) {
     if (isseparationchar(line[i++])) {
      if (i1==0)
       continue;
      break;
     }
    linepart[i1++]=line[i-1];
   }
   linepart[i1]='\0';

 return i;
}

// trim spaces at start and end of string
char* limitspaces(char *tstring)
{
  int i=0, n=0;
  static char ttstring[MAXBUFFER];
  
  while (isspace(tstring[i]) && i<strlen(tstring))
   ++i;
  for (;i<strlen(tstring);i++)
   ttstring[n++]=tstring[i];
  ttstring[n]='\0';
  i=strlen(ttstring)-1;
  while (isspace(ttstring[i]))
   --i;
  ttstring[++i]='\0';
  if (!strlen(ttstring))
   strcpy(ttstring, " ");
  
 return &ttstring[0];
}
    
// format char array and split to lines
int stringformattersplitter(char *text, char bufferinlines[MAXLINES][MAXBUFFER/2])
{
  int nlines=0;
  char ttext[MAXBUFFER], *token;
  const char *s = "\n";
  terminalnl=OFF; --linesdisplayed;
  
    strcpy(ttext, text);
    if ( ttext[strlen(ttext)-1] == '\n' || ttext[strlen(ttext)-1] == '\r' ) {
    terminalnl=ON;
    ++linesdisplayed;
   }
         
    token=strtok(ttext, s); // get first token
    while ( token != NULL && nlines < MAXLINES ) {
     strncpy( bufferinlines[nlines++], token, MAXBUFFER/2 );
     token=strtok(NULL, s);
    }
    
 return nlines;
}
    
// is printable character
int isprintablecharacter(int t)
{
  if (t>31 && t<127)
   return 1;
  
 return 0;
}
