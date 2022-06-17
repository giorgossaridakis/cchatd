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
#define MAXCONNECTIONS 100000
#define MAXCHANNELS 50000
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
enum { DEADLINK = -2, HIDDEN = -1, OFF = 0, ON }; // for .active members
extern const char *STATES[];
enum { NONE = 1, BASIC, INCREASED, FULL }; // log levels
extern const char *LOGLEVELS[];
enum { CLIENT = 1, SERVER };
extern const char *HIDDENNAME;
extern const char *NEWSFILE;
typedef unsigned int ui;
extern int comms;

int tfd=-1;

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
extern Connection* connections[MAXCONNECTIONS];
extern int nconnections;

typedef struct {
   char name[MAXNAME];
   char topic[MAXNAME];
   ui locked;
   ui active;
               } Channel;
extern Channel* channels[MAXCHANNELS];
extern int nchannels;

typedef struct {
    int fd;
    char ipaddress[MAXNAME];
    char port[MAXNAME];
    ui pipe;
    char bufferlines[MAXLINES][MAXBUFFER/2];
    int nbuffers;
               } OutConnection;
extern OutConnection* outconnections[MAXCHANNELS];
extern int noutconnections;

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
void announcedisconnectedusers();
void closeoutconnection(int connectionid);
void closeoutchannelconnection(int channelid);
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
int addchannel(char *name);
void listchannelusers(int connectionid, int flag);
void telluser(int connectionid, char *text, int c);
void telluseransi(int connectionid, char *text, int num, ...);
// external
extern ui logaction(char *text, int level);
extern Connection* nextconnection();
extern Channel* nextchannel();
extern void writeterminal(char *text, int c);
extern size_t readfileentry(int fd, char *line);
extern void outputtextfile(int connectionid, char *filename);
extern char* datetimenow();

// add connection
int addconnection()
{
  int i, fd_client, i1;
  char tline[MAXBUFFER];
  
   
    fd_client=accept(fd_skt, (struct sockaddr *) &sa2, &sa2_len);
    FD_SET(fd_client, &set);
     if ( fd_client > fd_hwm )
      fd_hwm = fd_client;
     
    if ( (i=whoisfd(fd_client)) != -1 ) {
     disconnect(fd_client); // precaution
     return 0;
    }
    for (i=0;i<nconnections;i++)
     if ( connections[i]->active == OFF )
      break;
     // server busy or mutes
    if ( i == nconnections && i < MAXCONNECTIONS - 2 ) { 
     if ( (connections[nconnections++]=nextconnection()) == NULL )
      --nconnections;
    }
    if ( i == nconnections || muteserver == ON ) {
     write( fd_client, "this system is currently unavailable\r\n", 38 );
     disconnect(fd_client);   
     return 0;
    }

    // add to connections
    connections[i]->fd=fd_client;
    strcpy( connections[i]->ipaddress, inet_ntoa(sa2.sin_addr) );
    connections[i]->port=ntohs(sa2.sin_port);
    sprintf( connections[i]->nickname, "user_%d", i);
    strcpy( connections[i]->date, datetimenow() );
    connections[i]->channel=CONSOLEID;
    connections[i]->invitation=OFF;
    connections[i]->operator=OFF;
    connections[i]->active=ON;
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
    sprintf( tline, "[ %s ] connected from %s port %d", connections[i]->nickname,  connections[i]->ipaddress, connections[i]->port );
    logaction(tline, BASIC);
    tellchannelusers( tline, GREEN, CONSOLEID, OFF );
    // read news file
    outputtextfile(i, (char *)NEWSFILE);
    
 return i;
}

// disconnect fd
void disconnect(int fd)
{
  int i;
  
  if ( tfd == fd ) // browser reload issue
   return;
  tfd=fd;
    
     i=whoisfd(fd);
     dismisschannel(i);
     telluser(i, "Goodbye!", RED);
     FD_CLR(fd, &set);
      if ( fd == fd_hwm ) {
       fd_hwm--;
      }
      close(fd);
      
      if ( muteserver == ON )
       return;
      connections[i]->active=DEADLINK;
      connections[i]->fd=-1;
}

// announce disconnected users
void announcedisconnectedusers()
{
  int i;
  char tline[MAXBUFFER];
  
   for (i=0;i<nconnections;i++) {
    if ( connections[i]->active == DEADLINK ) {
     sprintf(tline, "[ %s ] from %s port %d has disconnected", connections[i]->nickname, connections[i]->ipaddress, connections[i]->port);
     logaction(tline, BASIC);
     tellchannelusers( tline, GREEN, connections[i]->channel, OFF );
     connections[i]->active=OFF;
     tfd=-1;
    }
   }
   
}

// close outconnection
void closeoutconnection(int connectionid)
{
  close(outconnections[connections[connectionid]->channel]->fd);
  outconnections[connections[connectionid]->channel]->fd=-1;
}

// close outconnection by channel id
void closeoutchannelconnection(int channelid)
{
  close(outconnections[channelid]->fd);
  outconnections[channelid]->fd=-1;
}
  
// announcement
void announce(char *text, int c)
{
  int i;
  
   writeterminal(text, c);
   for (i=1;i<nconnections;i++) // 0 is reserved for console
    if ( connections[i]->active && connections[i]->active != DEADLINK ) // ansi here
     telluser(i, text, c);
    
}

// whoisnick in array
int whoisnick(char *nick)
{
  int i;
    
   for (i=0;i<nconnections;i++)
    if ( !strcmp(connections[i]->nickname, nick) && connections[i]->active && connections[i]->active != DEADLINK )
     break;
   if ( i == nconnections )
    return -1;

 return i;
}

// connection id from fd
int whoisfd(int fd)
{
 int i, id=-1;
 
  for (i=0;i<nconnections;i++) {
   if ( connections[i]->fd == fd ) {
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
  disconnect(connections[connectionid]->fd);
}

// users in channel
int channelusers(int channelid)
{
  int i1, users=0;

    if ( channels[channelid]->active == OFF )
     return 0;
    
    for (i1=0;i1<nconnections;i1++)
      if ( connections[i1]->channel == channelid && connections[i1]->active && connections[i1]->active != DEADLINK )
       ++users;

 return users;
}
        
// channel in array
int whatchannel(char *name)
{
  int i;
  
   for (i=0;i<nchannels;i++)
    if ( !strcmp(channels[i]->name, name) && channels[i]->active == ON )
     break;
   if ( i == nchannels )
    return -1;

 return i;
}
        
// activate channel
int addchannel(char *name)
{
  int id;
  
   for (id=0;id<nchannels;id++)
    if ( channels[id]->active == OFF )
     break;
   if ( id == nchannels && nchannels == MAXCHANNELS - 1)
    return -1;
   if ( id == nchannels ) {
    if ( (channels[nchannels++]=nextchannel()) == NULL ) {
     --nchannels;
     return -1;
    }
   }
   strcpy( channels[id]->name, name );
   strcpy( channels[id]->topic, "none" );
   channels[id]->locked=OFF;
   channels[id]->active=ON;
    
 return id;
}

// list channel users
void listchannelusers(int connectionid, int flag) // 0 all users, 1 only to connectionid
{
  int i;
  char tline[MAXBUFFER*10];
  
   sprintf( tline, "{%s}", channels[connections[connectionid]->channel]->name );
   if ( outconnections[connections[connectionid]->channel]->fd > -1 ) {
    strcat(tline, "*");
    char tline2[MAXBUFFER/2];
    snprintf(tline2, MAXBUFFER/2, ">%s:%s<", outconnections[connections[connectionid]->channel]->ipaddress, outconnections[connections[connectionid]->channel]->port);
    if ( connections[connectionid]->operator == ON || outconnections[connections[connectionid]->channel]->pipe == ON )
     strcat(tline, tline2);
   }
   
   for (i=0;i<nconnections;i++) {
    if ( connections[i]->active && connections[i]->active != DEADLINK && connections[i]->channel == connections[connectionid]->channel ) {
     strcat(tline, " [");
     if ( connections[i]->active == HIDDEN ) 
      strcat(tline, HIDDENNAME);
     else
      strcat(tline, connections[i]->nickname);
     if ( connections[i]->operator == ON )
      strcat(tline, "+");
     strcat(tline, "]");
    }
   }

   if ( flag == 0 ) { // all users in channel
    for (i=0;i<nconnections;i++) {
     if ( connections[i]->active && connections[i]->active != DEADLINK && connections[i]->channel == connections[connectionid]->channel ) {  
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
    
    if ( channelusers(connections[connectionid]->channel) > 1 || connections[connectionid]->channel == CONSOLEID || comms == 1 ) {
     return;
    }
    
     // deactivate channel now
     channels[connections[connectionid]->channel]->active=OFF;
     if ( outconnections[connections[connectionid]->channel]->fd > -1 ) {
      closeoutconnection(connectionid);
      outconnections[connections[connectionid]->channel]->pipe=OFF;
     }
        // dismiss invitations
     for (i=0;i<nconnections;i++)
      if ( connections[i]->invitation == connections[connectionid]->channel )
       connections[i]->invitation=CONSOLEID;

}

// tell user with color
void telluser(int connectionid, char *text, int c)
{ 
//   static char ttext[MAXBUFFER];
//   static int tconnectionid;
//   
//   if ( (strcmp(ttext, text)) ) {
//    strcpy(ttext, text);
//    tconnectionid=connectionid;
//   }
//   else if ( connectionid == tconnectionid )
//    return;
//     
  if ( connections[connectionid]->fd == STDOUT_FILENO ) {
   writeterminal( text, c );
   return;
  }
  if ( connections[connectionid]->active == OFF || connections[connectionid]->active == DEADLINK )
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
  
    if ( (write ( connections[connectionid]->fd, tline, strlen(tline) ) == -1) )
     disconnect(connections[connectionid]->fd);
}

// tell all users in channel
void tellchannelusers(char *text, int c, int channelid, int skipconsole)
{
  int i=skipconsole;
  
    for (;i<nconnections;i++)
     if ( connections[i]->active && connections[i]->active != DEADLINK && connections[i]->channel == channelid )
      telluser(i, text, c);    
}

// send a number of strings with ANSI references
void telluseransi(int connectionid, char *text, int num, ...)
{
  int i;
  va_list arguments;     
  char tline[MAXBUFFER];
  tline[0]='\0';
  
  if ( connections[connectionid]->active == OFF || connections[connectionid]->active == DEADLINK )
   return;
  
    va_start ( arguments, num );
    for (i=0;i<num;i++)
     strcat( tline, ANSISEQUENCES[ va_arg(arguments, int)] );
    va_end( arguments );
    strcat( tline, text);
    strcat (tline, ANSISEQUENCES[OLDCLASSRESET]);
    
    write( connections[connectionid]->fd, tline, strlen(tline) );   
}    
    
// total users
int nousers()
{
  int i, total=0;
  
   for (i=0;i<nconnections;i++)
    if ( connections[i]->active && connections[i]->active != DEADLINK )
     ++total;
    
 return total;
}

// total channels
int nochannels()
{
  int i, total=0;
  
   for (i=0;i<nchannels;i++)
    if ( channels[i]->active == ON )
     ++total;
    
 return total;
}
