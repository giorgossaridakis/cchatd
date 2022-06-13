// ncurses screen modules
#include <ncurses.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

WINDOW *win1; // ncurses window

enum { NORMAL=0, STANDOUT, UNDERLINE, REVERSE, BLINK, DIM, BOLD, PROTECT, INVISIBLE };
enum { BLACKONBLACK=1, BLACKONRED, BLACKONGREEN, BLACKONYELLOW, BLACKONBLUE, BLACKONMAGENTA,
       BLACKONCYAN, BLACKONWHITE,
       REDONBLACK, REDONRED, REDONGREEN, REDONYELLOW, REDONBLUE, REDONMAGENTA, REDONCYAN, REDONWHITE,
       GREENONBLACK, GREENONRED, GREENONGREEN, GREENONYELLOW, GREENONBLUE, GREENONMAGENTA, GREENONCYAN, GREENONWHITE,
       YELLOWONBLACK, YELLOWONRED, YELLOWONGREEN, YELLOWONYELLOW, YELLOWONBLUE, YELLOWONMAGENTA, YELLOWONCYAN, YELLOWONWHITE,
       BLUEONBLACK, BLUEONRED, BLUEONGREEN, BLUEONYELLOW, BLUEONBLUE, BLUEONMAGENTA, BLUEONCYAN, BLUEONWHITE,
       MAGENTAONBLACK, MAGENTAONRED, MAGENTAONGREEN, MAGENTAONYELLOW, MAGENTAONBLUE, MAGENTAONMAGENTA, MAGENTAONCYAN, MAGENTAONWHITE,
       CYANONBLACK, CYANONRED, CYANONGREEN, CYANONYELLOW, CYANONBLUE, CYANONMAGENTA, CYANONCYAN, CYANONWHITE,
       WHITEONBLACK, WHITEONRED, WHITEONGREEN, WHITEONYELLOW, WHITEONBLUE, WHITEONMAGENTA, WHITEONCYAN, WHITEONWHITE
      };
int BLACK=BLACKONBLACK, RED=REDONBLACK, GREEN=GREENONBLACK, YELLOW=YELLOWONBLACK, BLUE=BLUEONBLACK, MAGENTA=MAGENTAONBLACK, CYAN=CYANONBLACK, WHITE=WHITEONBLACK;
extern int COLORPAIRS[8][2];

#define MAXBUFFER 5120
#define CONNECTIONBUFFER 2624
#define MAXNAME 50
#define MAXWORDS 50
#define MAXCONNECTIONS 1000
#define MAXCHANNELS 500
#define MAXHISTORY 25000
#define MAXINPUT 80
#define MAXLINES 100
#define SPACE 32
#define CONSOLEID 0
#define MAXDEFINITION 11
#define SCREENX 81
#define SCREENY 24
enum { DEADLINK = -2, HIDDEN = -1, OFF = 0, ON }; // for .active members
extern const char *STATES[];
enum { NONE = 1, BASIC, INCREASED, FULL }; // log levels
extern const char *LOGLEVELS[];
enum { CLIENT = 1, SERVER };
typedef unsigned int ui;

// global variables
typedef struct {
   int fd;
   char ipaddress[MAXNAME];
   int port;
   char nickname[MAXNAME];
   char buffer[CONNECTIONBUFFER];
   char date[MAXNAME];
   ui channel;
   ui invitation;
   ui operator;
   ui active;
                } Connection;
extern Connection connections[MAXCONNECTIONS];

typedef struct {
   char name[MAXNAME];
   char topic[MAXNAME];
   ui locked;
   ui active;
               } Channel;
extern Channel channels[MAXCHANNELS];

typedef struct {
    int fd;
    char ipaddress[MAXNAME];
    char port[MAXNAME];
    ui pipe;
    char bufferlines[MAXLINES][MAXBUFFER/2];
    int nbuffers;
               } OutConnection;
extern OutConnection outconnections[MAXCHANNELS];


typedef struct {
    int pair;
    int attribute;
               } NCursesDefinition;
               
typedef struct {
    int attributes[SCREENX][SCREENY];
    int colors[SCREENX][SCREENY];
    int characters[SCREENX][SCREENY];
    int x, bx, y;
               } Screen;

// local variables
Screen screen, *screens[MAXHISTORY];
int screenspos=0, screenssize=0, linesdisplayed=0, previousscreen=OFF, myrow, mycol;
// extern variables
extern int loglevel, muteserver, comms, keephistory;
extern double VERSION;
extern char *serveraddress;
extern int serverport;
extern int monochrome;
extern char title[MAXINPUT];
extern int bufpos;
extern char buf[MAXBUFFER/2];

// function declarations
int initscreen();
void endscreen();
void color(int choice);
void gotoxy(int x, int y);
void attributes(int attribute);
void clearinputline();
void nextline();
void showscreen();
void cursor();
void writeterminal(char *text, int c);
void displayscreen(Screen *thisscreen);
void freescreens(int nscreens);
// extern
extern int isprintablecharacter(int t);
extern ui logaction(char *text, int level);
extern int channelusers(int channelid);
extern int nousers();
extern int nochannels();
extern NCursesDefinition* filteransisequence(char sequence[MAXDEFINITION]);
extern int snatchansitextpart(int startpt, char *text, char part[MAXBUFFER/2]);

// Initialize screen
int initscreen()
{
  win1=initscr();
  start_color();
  noecho();
  cbreak();
  keypad(win1, TRUE);
  mousemask(ALL_MOUSE_EVENTS, NULL);
  if (has_colors() == FALSE)
   monochrome=ON;
  curs_set(2);

  // black on others
  init_pair(BLACKONBLACK, COLOR_BLACK, COLOR_BLACK);
  init_pair(BLACKONRED, COLOR_BLACK, COLOR_RED);
  init_pair(BLACKONGREEN, COLOR_BLACK, COLOR_GREEN);
  init_pair(BLACKONYELLOW, COLOR_BLACK, COLOR_YELLOW);
  init_pair(BLACKONBLUE, COLOR_BLACK, COLOR_BLUE);
  init_pair(BLACKONMAGENTA, COLOR_BLACK, COLOR_MAGENTA);
  init_pair(BLACKONCYAN, COLOR_BLACK, COLOR_CYAN);
  init_pair(BLACKONWHITE, COLOR_BLACK, COLOR_WHITE);
  // red on others
  init_pair(REDONBLACK, COLOR_RED, COLOR_BLACK);
  init_pair(REDONRED, COLOR_RED, COLOR_RED);
  init_pair(REDONGREEN, COLOR_RED, COLOR_GREEN);
  init_pair(REDONYELLOW, COLOR_RED, COLOR_YELLOW);
  init_pair(REDONBLUE, COLOR_RED, COLOR_BLUE);
  init_pair(REDONMAGENTA, COLOR_RED, COLOR_MAGENTA);
  init_pair(REDONCYAN, COLOR_RED, COLOR_CYAN);
  init_pair(REDONWHITE, COLOR_RED, COLOR_WHITE);
  // green on others
  init_pair(GREENONBLACK, COLOR_GREEN, COLOR_BLACK);
  init_pair(GREENONRED, COLOR_GREEN, COLOR_RED);
  init_pair(GREENONGREEN, COLOR_GREEN, COLOR_GREEN);
  init_pair(GREENONYELLOW, COLOR_GREEN, COLOR_YELLOW);
  init_pair(GREENONBLUE, COLOR_GREEN, COLOR_BLUE);
  init_pair(GREENONMAGENTA, COLOR_GREEN, COLOR_MAGENTA);
  init_pair(GREENONCYAN, COLOR_GREEN, COLOR_CYAN);
  init_pair(GREENONWHITE, COLOR_GREEN, COLOR_WHITE);
  // yellow on others
  init_pair(YELLOWONBLACK, COLOR_YELLOW, COLOR_BLACK);
  init_pair(YELLOWONRED, COLOR_YELLOW, COLOR_RED);
  init_pair(YELLOWONGREEN, COLOR_YELLOW, COLOR_GREEN);
  init_pair(YELLOWONYELLOW, COLOR_YELLOW, COLOR_YELLOW);
  init_pair(YELLOWONBLUE, COLOR_YELLOW, COLOR_BLUE);
  init_pair(YELLOWONMAGENTA, COLOR_YELLOW, COLOR_MAGENTA);
  init_pair(YELLOWONCYAN, COLOR_YELLOW, COLOR_CYAN);
  init_pair(YELLOWONWHITE, COLOR_YELLOW, COLOR_WHITE);
  // blue on others
  init_pair(BLUEONBLACK, COLOR_BLUE, COLOR_BLACK);
  init_pair(BLUEONRED, COLOR_BLUE, COLOR_RED);
  init_pair(BLUEONGREEN, COLOR_BLUE, COLOR_GREEN);
  init_pair(BLUEONYELLOW, COLOR_BLUE, COLOR_YELLOW);
  init_pair(BLUEONBLUE, COLOR_BLUE, COLOR_BLUE);
  init_pair(BLUEONMAGENTA, COLOR_BLUE, COLOR_MAGENTA);
  init_pair(BLUEONCYAN, COLOR_BLUE, COLOR_CYAN);
  init_pair(BLUEONWHITE, COLOR_BLUE, COLOR_WHITE);
  // magenta on others
  init_pair(MAGENTAONBLACK, COLOR_MAGENTA, COLOR_BLACK);
  init_pair(MAGENTAONRED, COLOR_MAGENTA, COLOR_RED);
  init_pair(MAGENTAONGREEN, COLOR_MAGENTA, COLOR_GREEN);
  init_pair(MAGENTAONYELLOW, COLOR_MAGENTA, COLOR_YELLOW);
  init_pair(MAGENTAONBLUE, COLOR_MAGENTA, COLOR_BLUE);
  init_pair(MAGENTAONMAGENTA, COLOR_MAGENTA, COLOR_MAGENTA);
  init_pair(MAGENTAONCYAN, COLOR_MAGENTA, COLOR_CYAN);
  init_pair(MAGENTAONWHITE, COLOR_MAGENTA, COLOR_WHITE);
  // cyan on others
  init_pair(CYANONBLACK, COLOR_CYAN, COLOR_BLACK);
  init_pair(CYANONRED, COLOR_CYAN, COLOR_RED);
  init_pair(CYANONGREEN, COLOR_CYAN, COLOR_GREEN);
  init_pair(CYANONYELLOW, COLOR_CYAN, COLOR_YELLOW);
  init_pair(CYANONBLUE, COLOR_CYAN, COLOR_BLUE);
  init_pair(CYANONMAGENTA, COLOR_CYAN, COLOR_MAGENTA);
  init_pair(CYANONCYAN, COLOR_CYAN, COLOR_CYAN);
  init_pair(CYANONWHITE, COLOR_CYAN, COLOR_WHITE);
  // white on others
  init_pair(WHITEONBLACK, COLOR_WHITE, COLOR_BLACK);
  init_pair(WHITEONRED, COLOR_WHITE, COLOR_RED);
  init_pair(WHITEONGREEN, COLOR_WHITE, COLOR_GREEN);
  init_pair(WHITEONYELLOW, COLOR_WHITE, COLOR_YELLOW);
  init_pair(WHITEONBLUE, COLOR_WHITE, COLOR_BLUE);
  init_pair(WHITEONMAGENTA, COLOR_WHITE, COLOR_MAGENTA);
  init_pair(WHITEONCYAN, COLOR_WHITE, COLOR_CYAN);
  init_pair(WHITEONWHITE, COLOR_WHITE, COLOR_WHITE);
  
   attron(COLOR_PAIR(7));  
  
 return 0;
}

// close screen
void endscreen()
{
  endwin();
}

// change color
void color(int choice)
{  
   if ( monochrome == ON )
    return;
   attron(COLOR_PAIR(choice));
}

// gotoxy
void gotoxy(int x, int y)
{
  move(y-1, x-1);
}

// change attributes
void attributes(int attribute)
{
  static attr_t currentattributes;
  short cpair;
  
   attr_get(&currentattributes, &cpair, NULL);
    
   switch (attribute) {
    case NORMAL:
     attrset(A_NORMAL);
    break;
    case STANDOUT:
     attron(currentattributes | A_STANDOUT);
    break;
    case UNDERLINE:
     attron(currentattributes | A_UNDERLINE);
    break;
    case REVERSE:
     attron(currentattributes | A_REVERSE);
    break;
    case BLINK:
     attron(currentattributes | A_BLINK);
    break;
    case DIM:
     attron(currentattributes | A_DIM);
    break;
    case BOLD:
     attron(currentattributes | A_BOLD);
    break;
    case PROTECT:
     attron(currentattributes | A_PROTECT);
    break;
    case INVISIBLE:
     attron(currentattributes | A_INVIS);
    break;
   }

}

// clear input line
void clearinputline()
{
  int i;
  
   color(BLACK);
   for (i=1;i<81;i++) {
    gotoxy(i, myrow);
    addch(SPACE);
   }
   refresh();
}

// next line in screen array
void nextline()
{ 
  int i, i1;
  Screen tscreen;
  
    ++screen.y;
    screen.x=1;
    if ( keephistory == ON ) {
     ++linesdisplayed;
     if ( linesdisplayed == 22 ) { // put in routine here
      screens[screenssize]=malloc(sizeof (Screen));
      if ( screenssize > MAXHISTORY -2 || screens[screenssize] == 0 )
       freescreens(MAXHISTORY/5);
      *screens[screenssize++]=screen;
      ++screenspos;
      linesdisplayed=1;
     }
    }
    if ( screen.y > 23 ) { // scroll
       
     for (i=1;i<81;i++) {
      for (i1=2;i1<23;i1++) {
       tscreen.attributes[i][i1]=screen.attributes[i][i1+1];
       tscreen.colors[i][i1]=screen.colors[i][i1+1];
       tscreen.characters[i][i1]=screen.characters[i][i1+1];
      }
     }
       
     for (i=1;i<81;i++) {
      for (i1=2;i1<23;i1++) {
       screen.attributes[i][i1]=tscreen.attributes[i][i1];
       screen.colors[i][i1]=tscreen.colors[i][i1];
       screen.characters[i][i1]=tscreen.characters[i][i1];
      }
     }
    --screen.y;
    }
   
}

// show title and bar line
void showscreen()
{
  int i, tx;
  const int BARROW=23;
  
   tx=40-(strlen(title)/2);
   color(BLUE);
   attributes(STANDOUT);
   gotoxy(1, 1);
   for (i=1;i<81;i++)
    addch(SPACE);
   gotoxy(tx, 1);
   printw("%s", title);
   gotoxy(tx+strlen(title), 1);
   for (i=tx+strlen(title);i<81;i++)
    addch(SPACE);
   attributes(NORMAL);
   color(CYANONBLACK);
   gotoxy(1, BARROW);
   for (i=0;i<80;i++)
    addch(ACS_HLINE);
   if ( comms == 0 ) {
    // channel name
    color(YELLOW);
    gotoxy(3, BARROW);
    printw("[ch:%.10s]", channels[connections[CONSOLEID].channel].name);
    // locked or not
    if ( channels[connections[CONSOLEID].channel].locked == OFF ) {
     gotoxy(19, BARROW);
     color(BLUE);
     attributes(DIM);
     printw("[ locked]");
     attributes(NORMAL);
    }
    if ( channels[connections[CONSOLEID].channel].locked == ON ) {
     gotoxy(19, BARROW);
     color(BLUE);
     addch('[');
     attributes(BLINK);
     addch('*');
     attributes(NORMAL);
     color(BLUE);
     printw("locked]");
    }
    // muted server or not
    if ( muteserver == OFF ) {
     color(RED);
     gotoxy(29, BARROW);
     attributes(DIM);
     printw("[ mute]");
     attributes(NORMAL);
    }
    if ( muteserver == ON ) {
     color(RED);
     gotoxy(29, BARROW);
     addch('[');
     attributes(BLINK);
     addch('*');
     attributes(NORMAL);
     color(RED);
     printw("mute]");
     attributes(NORMAL);
    }
    // connected to out socket or not
    if ( outconnections[connections[CONSOLEID].channel].fd == -1 ) {
     color(GREEN);
     gotoxy(37, BARROW);
     attributes(DIM);
     printw("[ telnet]");
     attributes(NORMAL);
    }
    if ( outconnections[connections[CONSOLEID].channel].fd > -1 ) {
     color(GREEN);
     gotoxy(37, BARROW);
     addch('[');
     attributes(BLINK);
     addch('*');
     attributes(NORMAL);
     color(GREEN);
     printw("telnet]");
    }
    color(MAGENTA);
    gotoxy(47, BARROW);
    printw("[users:%d * total us/ch:%d/%d]", channelusers(connections[CONSOLEID].channel), nousers(), nochannels() );
   }
   if ( comms == 1 ) {
    // channel name
    color(YELLOW);
    gotoxy(48, BARROW);
    printw("[ch:%s]", channels[connections[CONSOLEID].channel].name);
    // keeping log or not
    if ( loglevel == NONE ) {
     color(MAGENTA);
     gotoxy(58, BARROW);
     attributes(DIM);
     printw("[ log]"); 
     attributes(NORMAL);
    }       
    if ( loglevel == FULL ) {
     color(MAGENTA);
     gotoxy(58, BARROW);
     addch('[');
     attributes(BLINK);
     addch('*');
     attributes(NORMAL);
     color(MAGENTA);
     printw("log]");
    }
    // connected to out socket or not
    if ( outconnections[connections[CONSOLEID].channel].fd == -1 ) {
     color(GREEN);
     gotoxy(65, BARROW);
     attributes(DIM);
     printw("[ connnection]"); 
     attributes(NORMAL);
    }
    if ( outconnections[connections[CONSOLEID].channel].fd > -1 ) {
     color(GREEN);
     gotoxy(65, BARROW);
     addch('[');
     attributes(BLINK);
     addch('*');
     attributes(NORMAL);
     color(GREEN);
     printw("connnection]");
    }
   }

   cursor();

  refresh();
}

// cursor imitation
void cursor()
{
   color(YELLOW);
   gotoxy(mycol, myrow);
   if ( mycol < bufpos + 1 )
    addch(buf[mycol-1]);
   else
    addch('_');
   gotoxy(mycol, myrow);
}

enum { ATTR=0, COL, CHAR };
// write to next terminal row 
void writeterminal(char *text, int c)
{
  int i, i1=0, myscreen[3][MAXBUFFER];
  char ansi[MAXDEFINITION];
  NCursesDefinition *tdefinition=malloc(sizeof (NCursesDefinition) );
  // default values
  tdefinition->attribute=NORMAL;
  tdefinition->pair=c;

   // remake array, where necessary
   for (i=0;i<strlen(text);i++) {
    if ( text[i] == '\e' ) {
     i+=snatchansitextpart(i, text, ansi); 
     tdefinition=filteransisequence(ansi); // pass through ansi.c
     continue;
    }
    if ( (isprintablecharacter(text[i])) == 0 )
     continue;
    myscreen[ATTR][i1]=tdefinition->attribute;
    myscreen[COL][i1]=tdefinition->pair;
    myscreen[CHAR][i1++]=text[i];
   }
  
     for (i=0;i<i1;i++) {
      screen.attributes[screen.x][screen.y]=myscreen[ATTR][i];
      screen.colors[screen.x][screen.y]=myscreen[COL][i];
      screen.characters[screen.x++][screen.y]=myscreen[CHAR][i];
      if ( screen.x > 80 ) {
       nextline();
      }
     }
     for (screen.bx=screen.x;screen.x<81;screen.x++) {
      screen.attributes[screen.x][screen.y]=NORMAL;
      screen.colors[screen.x][screen.y]=WHITE;
      screen.characters[screen.x][screen.y]=SPACE;
     }
     nextline(); // anyway, will reverse if terminalnl is OFF
     
     if ( previousscreen == ON )
      return;
     // now show
     displayscreen(&screen);
}

// display a Screen struct
void displayscreen(Screen *thisscreen)
{
  int i, i1;   
  
    attron(NORMAL);
    for (i=1;i<SCREENX;i++) {
     for (i1=2;i1<SCREENY-1;i1++) {
      attrset(thisscreen->attributes[i][i1]);
       attron(COLOR_PAIR(thisscreen->colors[i][i1]));
       gotoxy( i, i1 );
       addch( thisscreen->characters[i][i1] );
      }
     }
     refresh();
}

// clear screens
void freescreens(int nscreens)
{
  int i, tscreenspos=0;
  Screen* tscreens[MAXHISTORY];
  
   for (i=nscreens;i<MAXHISTORY;i++)
    tscreens[tscreenspos++]=screens[i];
   for (i=0;i<MAXHISTORY-nscreens;i++)
    screens[i]=tscreens[i];
   for (;i<MAXHISTORY;i++)
    memset(screens[i], 0, sizeof (Screen) );
   screenspos=screenssize=tscreenspos;
   screens[screenspos]=malloc(sizeof (Screen) );
   if ( screens[screenspos] == 0 )
    keephistory=OFF;
}
