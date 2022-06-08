// socket lib
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <errno.h>
#include <termios.h>
#include <stdarg.h>

// constants
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
#define SPACE 32
#define CONSOLEID 0

enum { ANSI_RESET = 90, ANSI_BOLD, ANSI_UNDERLINE, ANSI_BLINK, ANSI_REVERSE, ANSI_HIDDEN };
enum { ANSI_COLOR_BLACK = 0 , ANSI_COLOR_RED, ANSI_COLOR_GREEN, ANSI_COLOR_YELLOW, ANSI_COLOR_BLUE, ANSI_COLOR_MAGENTA, ANSI_COLOR_CYAN, ANSI_COLOR_WHITE };
enum { ANSI_BACKGROUND_BLACK = 56, ANSI_BACKGROUND_RED, ANSI_BACKGROUND_GREEN, ANSI_BACKGROUND_YELLOW, ANSI_BACKGROUND_BLUE, ANSI_BACKGROUND_MAGENTA, ANSI_BACKGROUND_CYAN, ANSI_BACKGROUND_WHITE };
enum { BLACKBACKGROUND = 56, REDBACKGROUMD, GREENBACKGROUND, YELLOWBACKGROUMD, BLUEBACKGROUND, MAGENTABACKGROUND, CYANBACKGROUND, WHITEBACKGROUND };
#define OLDCLASSRESET 88
enum { CRESET = 90, BOLDON, UNDERLINEON, BLINKON, REVERSEON, HIDDENON };
char *BANNEDMESSAGE="you have been banned from this system!\r\n";
enum { HIDDEN = -1, OFF = 0, ON }; // for .active members
extern const char *STATES[];
enum { NONE = 1, BASIC, INCREASED, FULL }; // log levels
extern const char *LOGLEVELS[];
enum { CLIENT = 1, SERVER };
extern const char *HIDDENNAME;
extern const char *NEWSFILE;
typedef unsigned int ui;

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

extern char* BANNEDFILE;
extern int loglevel, muteserver;
extern double VERSION;
extern char *serveraddress;
extern fd_set set, read_set;
extern struct sockaddr_in sa, sa2;
extern socklen_t sa2_len;
extern int serverport, fd_skt, fd_hwm;
extern const char *ANSISEQUENCES[96];
extern int COLORPAIRS[8][2];
extern int BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE;

// function declarations
int addconnection();
void disconnect(int fd);
void closeoutconnection(int connectionid);
void announce(char *text, int c);
void dismisschannel(int connectionid);
void tellchannelusers(char *text, int c, int channelid, int skipconsole);
int nousers();
int nochannels();
int whoisnick(char *nick);
int whoisfd(int fd);
void kick(int connectionid);
int channelusers(int channelid);
int whatchannel(char *name);
int newchannel(char *name);
void listchannelusers(int connectionid, int flag);
void telluser(int connectionid, char *text, int c);
void telluseransi(int connectionid, char *text, int num, ...);
// external
extern ui logaction(char *text, int level);
extern void writeterminal(char *text, int c);
extern size_t readfileentry(int fd, char *line);
void outputtextfile(int connectionid, char *filename);



// add connection
int addconnection()
{
  int i, fd_client, i1;
  char tline[MAXBUFFER];
  
   
    fd_client=accept(fd_skt, (struct sockaddr *) &sa2, &sa2_len);
    FD_SET(fd_client, &set);
     if (fd_client > fd_hwm)
      fd_hwm = fd_client;
     
    if ( (i=whoisfd(fd_client)) != -1 ) {
     disconnect(fd_client); // precaution
     return 0;
    }
    for (i=0;i<MAXCONNECTIONS;i++)
     if ( connections[i].active == OFF )
      break;
     // server busy or mutes
    if ( i == MAXCONNECTIONS || muteserver == ON ) {
     disconnect(fd_client);   
     return 0;
    }

    // add to connections
    connections[i].fd=fd_client;
    strcpy( connections[i].ipaddress, inet_ntoa(sa2.sin_addr) );
    connections[i].port=ntohs(sa2.sin_port);
    sprintf( connections[i].nickname, "user_%d", i);
    connections[i].channel=CONSOLEID;
    connections[i].invitation=OFF;
    connections[i].operator=OFF;
    connections[i].active=ON;
    // check if banned
    i1=open(BANNEDFILE, O_RDONLY);
    while ( i1 != -1 && (readfileentry(i1, tline)) ) {
     if ( !strcmp(tline, inet_ntoa(sa2.sin_addr)) ) {
      telluser(whoisfd(fd_client), BANNEDMESSAGE, RED);
      disconnect(fd_client);
      snprintf( tline, MAXBUFFER, "banned ip [ %s ] tried to connect\r\n", inet_ntoa(sa2.sin_addr) );
      logaction(tline, BASIC);
      return 0;
     }
    }
    // welcome user
    sprintf( tline, "*cchatd version %.2f* host is %s [/help for help]\r\n", VERSION, serveraddress );
    telluser(whoisfd(fd_client), tline, BLUE);
    sprintf( tline, "[ %s ] connected from %s port %d", connections[i].nickname,  connections[i].ipaddress, connections[i].port );
    logaction(tline, BASIC);
    tellchannelusers( tline, GREEN, CONSOLEID, CONSOLEID );
    // read news file
    outputtextfile(i, (char *)NEWSFILE);
    
 return i;
}

// disconnect fd
void disconnect(int fd)
{
  int i;
  char buf[MAXBUFFER];
    
     i=whoisfd(fd);
     telluser(i, "Goodbye!", RED);
     dismisschannel(connections[i].channel);
     FD_CLR(fd, &set);
      if (fd == fd_hwm) {
       fd_hwm--;
      }
      close(fd);
      if ( muteserver == ON )
       return;
      sprintf(buf, "[ %s ] from %s port %d has disconnected\r\n", connections[i].nickname, connections[i].ipaddress, connections[i].port);
      logaction(buf, BASIC);
      writeterminal(buf, GREEN);
      connections[i].active=OFF;
      connections[i].fd=-1;
      for (i=1;i<MAXCONNECTIONS;i++)
       if ( connections[i].active )
        write( connections[i].fd, buf, strlen(buf) );
}

// close outconnection
void closeoutconnection(int connectionid)
{
  close(outconnections[connectionid].fd);
  outconnections[connectionid].fd=-1;
}
  
// announcement
void announce(char *text, int c)
{
  int i;
  
   writeterminal(text, c);
   for (i=1;i<MAXCONNECTIONS;i++) // 0 is reserved for console
    if ( connections[i].active ) // ansi here
     telluser(i, text, c);
    
}

// whoisnick in array
int whoisnick(char *nick)
{
  int i;
    
   for (i=0;i<MAXCONNECTIONS;i++)
    if ( !strcmp(connections[i].nickname, nick) && connections[i].active )
     break;
   if ( i == MAXCONNECTIONS )
    return -1;

 return i;
}

// connection id from fd
int whoisfd(int fd)
{
 int i, id=-1;
 
  for (i=0;i<MAXCONNECTIONS;i++) {
   if ( connections[i].fd == fd ) {
    id=i;
    break;
   }
  }
  
 return id;
}
        
// kick user out
void kick(int connectionid)
{
  telluser(connectionid, "you have been kicked out!", RED);
  disconnect(connections[connectionid].fd);
}

// users in channel
int channelusers(int channelid)
{
  int i1, users=0;

    if ( channels[channelid].active == OFF )
     return 0;
    
    for (i1=0;i1<MAXCONNECTIONS;i1++)
      if ( connections[i1].channel == channelid && connections[i1].active )
       ++users;

 return users;
}
        
// channel in array
int whatchannel(char *name)
{
  int i;
  
   for (i=0;i<MAXCHANNELS;i++)
    if ( !strcmp(channels[i].name, name) && channels[i].active == 1 )
     break;
   if ( i == MAXCHANNELS )
    return -1;

 return i;
}
        
// activate channel
int newchannel(char *name)
{
  int id;
  
   for (id=0;id<MAXCHANNELS;id++)
    if ( channels[id].active == 0 )
     break;
   if ( id == MAXCHANNELS )
    return -1;
   strcpy( channels[id].name, name );
   strcpy( channels[id].topic, "none" );
   channels[id].locked=OFF;
   channels[id].active=ON;
    
 return id;
}

// list channel users
void listchannelusers(int connectionid, int flag) // 0 all users, 1 only to connectionid
{
  int i;
  char tline[MAXBUFFER*10];
  
   sprintf( tline, "{%s}", channels[connections[connectionid].channel].name );
   if ( outconnections[connections[connectionid].channel].fd > -1 ) {
    strcat(tline, "*");
    char tline2[MAXBUFFER/2];
    snprintf(tline2, MAXBUFFER/2, ">%s:%s<", outconnections[connections[connectionid].channel].ipaddress, outconnections[connections[connectionid].channel].port);
    if ( connections[connectionid].operator == ON || outconnections[connections[connectionid].channel].pipe == ON )
     strcat(tline, tline2);
   }
   
   for (i=0;i<MAXCONNECTIONS;i++) {
    if ( connections[i].active && connections[i].channel == connections[connectionid].channel ) {
     strcat(tline, " [");
     if ( connections[i].active == HIDDEN ) 
      strcat(tline, HIDDENNAME);
     else
      strcat(tline, connections[i].nickname);
     if ( connections[i].operator == ON )
      strcat(tline, "+");
     strcat(tline, "]");
    }
   }

   if ( flag == 0 ) { // all users in channel
    for (i=0;i<MAXCONNECTIONS;i++) {
     if ( connections[i].active == 1 && connections[i].channel == connections[connectionid].channel ) {  
      telluser(i, tline, CYAN);
     }
    }
   }
   if ( flag == 1 )
    telluser(connectionid, tline, CYAN);   
}
// depart channel
void dismisschannel(int connectionid)
{
  int i;
  char tline[MAXBUFFER];
    
    if ( channelusers(connections[connectionid].channel) > 1 )
     return;
    // deactivate channel, if need be
    if ( (channelusers(connections[connectionid].channel) == 1) && connections[connectionid].channel != CONSOLEID ) {
     channels[connections[connectionid].channel].active=0;
     if ( outconnections[connections[connectionid].channel].fd > -1 ) {
      close(outconnections[connections[connectionid].channel].fd);
      outconnections[connections[connectionid].channel].fd=-1;
      outconnections[connections[connectionid].channel].pipe=OFF;
     }
        // dismiss invitations
     for (i=0;i<MAXCONNECTIONS;i++)
      if ( connections[i].invitation == connections[connectionid].channel )
       connections[i].invitation=0;
    }
    // inform remaining users
    sprintf(tline, "%s has left the channel", connections[connectionid].nickname);
    for (i=0;i<MAXCONNECTIONS;i++)
     if ( connections[i].active && connections[i].channel == connections[connectionid].channel && connectionid != i )
      telluser(i, tline, GREEN);
}

// tell user
void telluser(int connectionid, char *text, int c)
{  
  if ( connections[connectionid].fd == STDOUT_FILENO ) {
   writeterminal( text, c );
   return;
  }
  if ( connections[connectionid].active == OFF )
   return;
  char tline[MAXBUFFER];
  int i, ansipair=7;
    
    for (i=0;i<8;i++)
     if ( c == COLORPAIRS[i][0] )
      break;
    if ( i < 8 )
     ansipair=COLORPAIRS[i][1];
    sprintf(tline, "%s", ANSISEQUENCES[ansipair]);
    strcat(tline, text);
    strcat(tline, ANSISEQUENCES[OLDCLASSRESET]);
    strcat(tline, "\r\n");
  
    write( connections[connectionid].fd, tline, strlen(tline) );
}

// tell all users in channel
void tellchannelusers(char *text, int c, int channelid, int skipconsole)
{
  int i=skipconsole;
  
    for (;i<MAXCONNECTIONS;i++)
     if ( connections[i].active && connections[i].channel == channelid )
      telluser(i, text, c);    
}

void telluseransi(int connectionid, char *text, int num, ...)
{
  int i;
  va_list arguments;     
  char tline[MAXBUFFER];
  tline[0]='\0';
  
  if ( connections[connectionid].active == OFF )
   return;
  
    va_start ( arguments, num );
    for (i=0;i<num;i++)
     strcat( tline, ANSISEQUENCES[ va_arg(arguments, int)] );
    va_end( arguments );
    strcat( tline, text);
    strcat (tline, ANSISEQUENCES[OLDCLASSRESET]);
    
    write( connections[connectionid].fd, tline, strlen(tline) );    
}    
    
// total users
int nousers()
{
  int i, total=0;
  
   for (i=0;i<MAXCONNECTIONS;i++)
    if ( connections[i].active )
     ++total;
    
 return total;
}

// total channels
int nochannels()
{
  int i, total=0;
  
   for (i=0;i<MAXCHANNELS;i++)
    if ( channels[i].active == 1 )
     ++total;
    
 return total;
}