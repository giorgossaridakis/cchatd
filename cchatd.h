// cchatd header

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <errno.h>
#include <termios.h>
#include <time.h>
#include <stdbool.h>
#include <ncurses.h>

// constants
typedef enum {OT_INT, OT_LINGER, OT_TIMEVAL} OPT_TYPE;
enum { NORMAL=0, STANDOUT, UNDERLINE, REVERSE, BLINK, DIM, BOLD, PROTECT, INVISIBLE };
#define MAXBUFFER 5120
#define CONNECTIONBUFFER 2624
#define MAXNAME 50
#define MAXWORDS 50
#define MAXCONNECTIONS 1000
#define MAXCHANNELS 500
#define MAXHISTORY 25000
#define MAXINPUT 80
#define MAXLINES 100
#define SCREENX 81
#define SCREENY 24
// keyboard
#define SPACE 32
#define UP 259
#define DOWN 258
#define PGUP 339
#define LEFT 260
#define RIGHT 261
#define PGDN 338
#define BACKSPACE 263
#define BACKSPACE2 127
#define DELETE 330
#define INSERT 331
#define HOME 262
#define END 360
#define ESC 27
#define TAB 9
#define SHIFT_TAB 353

const char *CONSOLE="console";
const char *MAINHALL="zero";
const char *MAINHALLTOPIC="Welcoming Channel - General Discussions";
const char *BANNEDFILE="cchatd.banned";
const char *LOGFILE="cchatd.log";
const char *PORTFILE="cchatd.port";
const char *NEWSFILE="cchatd.news";
const char *HIDDENNAME="someone";
const char *EVERYBODY="all";
const char *HIDDENIP="X.X.X.X";
#define CONSOLEID 0
enum { HIDDEN = -1, OFF = 0, ON }; // for .active members
const char *STATES[] = { "off", "on" };
enum { NONE = 1, BASIC, INCREASED, FULL }; // log levels
const char *LOGLEVELS[] = { "empty", "none", "basic", "increased", "full" };
enum { CLIENT = 1, SERVER };
typedef unsigned int ui;
const int SERVERPORT=23; // probably locked, will go to client mode if no port in PORTFILE

// global variables
typedef struct {
   int fd;
   char ipaddress[MAXNAME];
   int port;
   char nickname[MAXNAME];
   char buffer[CONNECTIONBUFFER];
   ui channel;
   ui invitation;
   ui operator;
   ui active;
                } Connection;
Connection connections[MAXCONNECTIONS];

typedef struct {
   char name[MAXNAME];
   char topic[MAXNAME];
   ui locked;
   ui active;
               } Channel;
Channel channels[MAXCHANNELS];

typedef struct {
    int fd;
    char ipaddress[MAXNAME];
    char port[MAXNAME];
    ui pipe;
    char bufferlines[MAXLINES][MAXBUFFER/2];
    int nbuffers;
               } OutConnection;
OutConnection outconnections[MAXCHANNELS];

typedef struct {
    int attributes[SCREENX][SCREENY];
    int colors[SCREENX][SCREENY];
    int characters[SCREENX][SCREENY];
    int x, bx, y;
               } Screen;

// local variables
int fd_skt, fd_hwm=0, serverport, loglevel=INCREASED, muteserver=OFF, monochrome=OFF, comms=0, keephistory=ON;
char title[MAXINPUT];
FILE* logfile;
char* serveraddress;
fd_set set, read_set;
ssize_t nread;
time_t rawtime;
struct tm* timeinfo;
char buf[MAXBUFFER/2], clientbuffer[MAXBUFFER];
char* keyboardhistory[MAXHISTORY];
int keyboardhistorypos=0, keyboardhistorysize=0;
int terminalnl=ON;
int bufpos;
int disconnectedusers[MAXCONNECTIONS], ndisconnectedusers;
// extern
extern WINDOW *win1;
extern Screen screen, *screens[MAXHISTORY];
extern int  screenspos, screenssize, previousscreen, mycol, myrow;
extern int BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE;

// function declarations
// local
void initvariables();
int readkeyboard(WINDOW *twin);
void freekeyboardhistory(int lines);
ui logaction(char *text, int level);
ui logoutconnectionaction(char *text, int level);
void outputtextfile(int connectionid, char *filename);
void endcchatd();
void INThandler(int sig);
// extern
//screen.c
extern int initscreen();
extern void endscreen();
extern void color(int choice);
extern void gotoxy(int x, int y);
extern void attributes(int attribute);
extern void clearinputline();
extern void nextline();
extern void showscreen();
extern void cursor();
extern void writeterminal(char *text, int c);
extern void displayscreen(Screen *thisscreen);
//command.c
extern char* limitspaces(char *tstring);
extern int assignstringvaluestoarray(char *line, char array[MAXWORDS][MAXBUFFER/2], int entries);
extern size_t readfileentry(int fd, char *line);
extern size_t readfileentry(int fd, char *line);
extern int stringformattersplitter(char *text, char bufferinlines[MAXLINES][MAXBUFFER/2]);
extern int isprintablecharacter(int t);
//socket.c
extern struct sockaddr_in sa, sa2;
extern socklen_t sa2_len;
extern int initcomms();
extern int clientsocket(char* host, char* port);
extern char *ipaddress();
//socklib.c 
extern int addconnection();
extern void disconnect(int fd);
extern void announcedisconnectedusers();
extern void closeoutconnection(int connectionid);
extern void announce(char *text, int c);
extern void dismisschannel(int connectionid);
extern void tellchannelusers(char *text, int c, int channelid, int skipconsole);
extern int nousers();
extern int nochannels();
extern void telluser(int connectionid, char *text, int c);
extern int whoisnick(char *nick);
extern int whoisfd(int fd);
void kick(int connectionid);
extern int channelusers(int channelid);
extern int whatchannel(char *name);
extern int newchannel(char *name);
extern void listchannelusers(int connectionid, int flag);
//parser.c 
extern ui fdcommands(char* command, int connectionid);
extern ui parsecommand(char words[MAXWORDS][MAXBUFFER/2], int nwords, int connectionid);
extern void joinchannel(int connectionid, int nextchannel);
