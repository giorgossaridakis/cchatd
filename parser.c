// user commands parser
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>

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
#define CONSOLEID 0
enum { DEADLINK = - 2, HIDDEN = -1, OFF = 0, ON }; // for .active members
extern const char *STATES[];
enum { NONE = 1, BASIC, INCREASED, FULL }; // log levels
extern const char *LOGLEVELS[];
enum { CLIENT = 1, SERVER };
typedef unsigned int ui;
extern char* BANNEDFILE;
extern double VERSION;
extern const char *CONSOLE;
extern const char *HIDDENNAME;
extern const char *HIDDENIP;
extern const char *EVERYBODY;
extern char cchatddatetime[MAXNAME];

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

// extern
extern int loglevel, muteserver, comms;
extern ssize_t nread;
extern time_t rawtime;
extern struct tm * timeinfo;
extern int BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE;
extern FILE* logfile;
extern const char *LOGFILE;
extern char* datetimenow();

// function declarations
ui fdcommands(char* command, int connectionid);
ui parsecommand(char words[MAXWORDS][MAXBUFFER/2], char bwords[MAXWORDS][MAXBUFFER/2], int nwords, int connectionid);
void joinchannel(int connectionid, int nextchannel);
extern char* limitspaces(char *tstring);
extern int assignstringvaluestoarray(char *line, char array[MAXWORDS][MAXBUFFER/2], int entries);
extern int stringformattersplitter(char *text, char bufferinlines[MAXLINES][MAXBUFFER]);
extern int isprintablecharacter(int t);
extern ui logaction(char *text, int level);
extern void telluser(int connectionid, char *text, int c);
extern int whoisnick(char *nick);
extern int whoisfd(int fd);
extern void announce(char *text, int c);
extern void endcchatd();
extern void disconnect(int fd);
extern void kick(int connectionid);
extern int whatchannel(char *name);
extern int addchannel(char *name);
extern void listchannelusers(int connectionid, int flag);
extern void dismisschannel(int connectionid);
extern void tellchannelusers(char *text, int c, int channelid, int skipconsole);
extern int channelusers(int channelid);
extern int clientsocket(char* host, char* port);
extern void closeoutconnection(int connectionid);
extern void outputtextfile(int connectionid, char *filename);

// function bodies
// command process
ui fdcommands(char* command, int connectionid)
{
  int i, nwords;
  char tcommand[MAXBUFFER], tline[MAXBUFFER];
  char words[MAXWORDS][MAXBUFFER/2], bwords[MAXWORDS][MAXBUFFER/2];
  strcpy( tcommand, (command) );
  if ( strlen(tcommand) < 2 )
   return 0;
  if ( tcommand[0] != '/' ) {
   return 0;
  }
  if ( tcommand[0] == '/' && tcommand[1] == '/' ) { // send to outconnection
   for (i=1;i<strlen(tcommand);i++) {
    command[i-1]=tcommand[i];
   }
   command[i-1]='\0';
   return 0;
  }

    assignstringvaluestoarray(tcommand, bwords, MAXWORDS); // keep a backup
    for (i=0;i<strlen(tcommand)-1;i++)
     tcommand[i]=tolower( tcommand[i+1] );
    tcommand[i]='\0';
  
    nwords=assignstringvaluestoarray(tcommand, words, MAXWORDS);
    snprintf(tline, MAXBUFFER, "%s->", connections[connectionid]->nickname);
    strcat(tline, tcommand);
    logaction(tline, INCREASED);
    parsecommand(words, bwords, nwords, connectionid);
    // clear buffers
    memset(command, 0, MAXBUFFER/2 );
    for (i=0;i<nwords;i++) 
     memset( words[i], 0, MAXBUFFER/2 );
  
 return nwords;
}

// command parser
ui parsecommand(char words[MAXWORDS][MAXBUFFER/2], char bwords[MAXWORDS][MAXBUFFER/2], int nwords, int connectionid)
{
  int i, i1, i2;
  char tline[MAXBUFFER], title[MAXNAME];
  enum { HELP=0, NICK, WHO, MSG, LIST, JOIN, LEAVE, INVITE, TOPIC, LOCK, UNLOCK, REPORT, NAMES, QUIT, HIDE, ANNOUNCE, KICK, OP, DEOP, BAN, READ, LOG, OPEN, CLOSE, PIPE, MUTESERVER };
  char *COMMANDS[] = { "help", "nick", "who", "msg", "list", "join", "leave", "invite", "topic", "lock", "unlock", "report", "names", "quit", "hide", "announce", "kick", "op", "deop", "ban", "read", "log" , "open", "close", "pipe", "mute" };
  
   for (i=0;i<26;i++)
    if ( !strcmp(words[0], COMMANDS[i]) )
     break;
   // parse for errors in request
   if ( ( comms == 1 && i != OPEN && i != CLOSE && i != QUIT && i != LOG && i != HELP ) || i == 26 || ( i > HIDE && connections[connectionid]->operator == OFF ) || ( i > OP && connectionid != CONSOLEID ) ) {
    telluser(connectionid, "unknown command", RED);
    return 0;
   }
  
   // parse commands
   if ( i == HELP ) {
    if ( nwords != 1 ) {
     telluser(connectionid, "<help> ->no parameters<-", RED);
     return 0;
    }
    if ( comms == 1 ) { // telnet client
     telluser(CONSOLEID, "commands are: help (this help), open <server> <port>, close (connection), log <level>, quit [ console keys TAB, SHIFT_TAB, UP, DOWN, PG_UP, PG_DOWN ]", YELLOW);
     return 0;
    }
    strcpy(tline, "commands are: help (this help), nick <new nickname>, who, msg <nick> <message>, list (channels), join <channel>, leave (this channel), invite <nickname>, topic <channel topic>, lock (this channel), unlock (this channel), report <your report>, names (in this channel), quit");
    if ( connections[connectionid]->operator == ON )
     strcat(tline, ", hide, announce <message>, kick <nick>, op <nick>");
    if ( connectionid == CONSOLEID) 
     strcat(tline, ", deop <nick>, ban <nick>, read <target nickname|all> <filename>, log <level>, open <server> <port>, close, pipe, mute [ console keys TAB, SHIFT_TAB, UP, DOWN, PG_UP, PG_DOWN ]");
    strcat(tline, "");
    telluser(connectionid, tline, YELLOW);
   }
  
   if ( i == NICK ) {
    if ( nwords != 2 || !strcmp(words[1], HIDDENNAME) || !strcmp(words[1], EVERYBODY) || (connectionid != CONSOLEID && !strcmp(words[1], CONSOLE)) ) {
     telluser(connectionid, "<nick> new (available) nickname", RED);
     return 0;
    }
    if ( (whoisnick(words[1])) != -1 ) {
     telluser(connectionid, "that nickname is already taken", RED);
     return 0;
    }
    sprintf( tline, "%s is now known as %s", connections[connectionid]->nickname, words[1] );
    logaction(tline, INCREASED);
    if ( connections[connectionid]->active != HIDDEN )
     announce(tline, GREEN);
    strcpy( connections[connectionid]->nickname, words[1] );
   }
   if ( i == QUIT ) {
    if ( nwords != 1 ) {
     return 0;
    }
    if ( connectionid == 0 )
     endcchatd();
    else
     disconnect(connections[connectionid]->fd);
   }
   
   if ( i == ANNOUNCE ) {
    if ( nwords < 2 ) {
     telluser(connectionid, "<announce> your announcement", RED);
     return 0;
    }
    sprintf(tline, "***\aannouncement from [ %s ]*** ", connections[connectionid]->nickname);
    for (i1=1;i1<nwords;i1++) {
     strcat(tline, bwords[i1]);
     strcat(tline, " ");
    }
    strcat(tline, "");
    logaction(tline, INCREASED);
    announce(tline, CYAN);
   }
   if ( i == BAN ) {
    if ( (i1=open(BANNEDFILE, O_WRONLY| O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR))==-1 ) {
     telluser(connectionid, "file io error", RED);
     return 0;
    }
    if ( nwords != 2 || (i2=whoisnick(words[1]))==-1 || (i2==CONSOLEID) ) {
     telluser(connectionid, "<ban> nickname", RED);
     return 0;
    }
    write(i1, connections[i2]->ipaddress, strlen(connections[i2]->ipaddress) );
    write(i1, "\n", 1);
    close(i1);
    kick(i2);
    sprintf(tline, "%s from %s is banned", connections[i2]->nickname, connections[i2]->ipaddress);
    telluser(connectionid, tline, RED);
   }
   if ( i == KICK ) {
    if ( nwords != 2 || (i2=whoisnick(words[1]))==-1 || (i2==CONSOLEID) ) {
     telluser(connectionid, "<kick> nickname", RED);
     return 0;
    }
    snprintf(tline, MAXBUFFER, "%s has been kicked out", words[1]);
    tellchannelusers(tline, GREEN, connections[i2]->channel, OFF);
    kick(i2);
   }
   if ( i == WHO ) {
    if (nwords != 1) {
     telluser(connectionid, "<who> ->no parameters<-", RED);
     return 0;
    }
    time (&rawtime);
    timeinfo = localtime (&rawtime);
    strftime(title, MAXBUFFER, "%H:%M:%S", timeinfo);
    snprintf(tline, MAXBUFFER, "cchatd version %.2f local time: %s", VERSION, title);
    strcpy(title, tline);
    telluser(connectionid, title, BLUE);
    telluser(connectionid, "----------------------------------------", BLUE);
    for (i1=0;i1<nconnections;i1++) {
     if ( connections[i1]->active && connections[i1]->active != DEADLINK ) {
      snprintf(tline, MAXBUFFER, "%s%s{%s}%s%s:%d (%s)", (connections[i1]->active == HIDDEN) ? HIDDENNAME : connections[i1]->nickname, (connections[i1]->operator == ON ) ? "[op] " : " " , channels[connections[i1]->channel]->name, (outconnections[connections[i1]->channel]->fd > -1) ? "* " : " ", (connections[i1]->active == HIDDEN) ? HIDDENIP : connections[i1]->ipaddress, (connections[i1]->active == HIDDEN) ? 0 : connections[i1]->port, connections[i1]->date );
      telluser(connectionid, tline, CYAN);
     }
    }
    snprintf(tline, MAXBUFFER, "--> online since %s <--", cchatddatetime);
    telluser(connectionid, tline, YELLOW);
   }
   if ( i == OP ) {
    if ( nwords != 2 || (i2=whoisnick(words[1]))==-1 ) {
     telluser(connectionid, "<op> nickname", RED);
     return 0;
    }
    if ( connections[i2]->operator == ON ) {
     telluser(connectionid, "user is already operator", RED);
     return 0;
    }
    connections[i2]->operator=ON;
    sprintf(tline, "%s has been OPed", connections[i2]->nickname);
    tellchannelusers( tline, GREEN, connections[connectionid]->channel, OFF );
    sprintf(tline, "you have been OPed by [ %s ]", connections[connectionid]->nickname);
    telluser(i2, tline, GREEN);
   }
   if ( i == DEOP ) {
    if ( nwords != 2 || (i2=whoisnick(words[1]))==-1 || (i2==CONSOLEID) ) {
     telluser(connectionid, "<deop> nickname", RED);
     return 0;
    }
    if ( connections[i2]->operator == OFF ) {
     sprintf(tline, "%s is not an operator", connections[i2]->nickname);
     telluser(connectionid, tline, RED);
     return 0;
    }
    connections[i2]->operator=OFF;
    sprintf(tline, "%s has been de-OPed", connections[i2]->nickname);
    tellchannelusers( tline, GREEN, CONSOLEID, OFF );
    sprintf(tline, "you have been de-OPed by [ %s ]", connections[connectionid]->nickname);
    telluser(i2, tline, GREEN);
   }
   if ( i== JOIN ) {
    if ( nwords != 2 ) {
     telluser(connectionid, "<join> which channel ?", RED);
     return 0;
    }
    if ( (i1=whatchannel(words[1])) == -1 )
     if ( (i1=addchannel(words[1])) == -1 ) {
     telluser(connectionid, "all channels are occupied, try later", RED);
     return 0;
    }
    if ( channels[i1]->locked == ON && connections[connectionid]->invitation != i1 ) {
     telluser(connectionid,  "channel is private, you haven't been invited!", RED);
     return 0;
    }
    if ( connections[connectionid]->channel == i1 ) {
     telluser(connectionid, "you are already in the channel", RED);
     return 0;
    }
    joinchannel(connectionid, i1);
   }
   if ( i == LEAVE ) {
    if ( nwords != 1 || connections[connectionid]->channel == CONSOLEID ) {
     telluser(connectionid, "<leave> any channel except zero", RED);
     return 0;
    }
    joinchannel(connectionid, CONSOLEID);
   }
   if ( i == INVITE ) {
    if ( nwords != 2 ) {
     telluser(connectionid, "<invite> nickname", RED);
     return 0;
    }
    if ( connectionid == (i2=whoisnick(words[1])) ) {
     telluser(connectionid, "inviting yourself ?", RED);
     return 0;
    }
    if ( i2 == -1 ) {
     telluser(connectionid, "no such user online", RED);
     return 0;
    }
    sprintf(tline, "%s is inviting you to %s", connections[connectionid]->nickname, channels[connections[connectionid]->channel]->name);
    if ( connections[i2]->channel != connections[connectionid]->channel ) {
     connections[i2]->invitation=connections[connectionid]->channel;
     telluser(i2, tline, CYAN);
     telluser(connectionid, "invitation sent", CYAN);
    }
    else
     telluser(connectionid, "you are in the same channel!", RED);
   }
   if ( i == TOPIC ) {
    if ( nwords < 2 ) {
     telluser(connectionid, "<topic> new channel topic", RED);
     return 0;
    }
    if ( connections[connectionid]->channel == CONSOLEID && connectionid != CONSOLEID ) {
     telluser(connectionid, "<topic> not possible", RED);
     return 0;
    }
    strcpy( title, "new channel topic:" );
    tline[0]='\0';
    for (i1=1;i1<nwords;i1++) {
     strcat(tline, bwords[i1]);
     strcat(tline, " ");
    }
    strcpy( channels[connections[connectionid]->channel]->topic, limitspaces(tline) );
    strcat(tline, "");
    for (i2=0;i2<nconnections;i2++) {
     if ( connections[i2]->active && connections[i2]->active != DEADLINK && connections[i2]->channel == connections[connectionid]->channel ) {
      telluser(i2, title, BLUE);
      telluser(i2, tline, CYAN);
     }
    }
   }
   if ( i == LIST ) {
    if (nwords != 1) {
     telluser(connectionid, "<list> ->no parameters<-", RED);
     return 0;
    }
    time (&rawtime);
    timeinfo = localtime (&rawtime);
    strftime(title, MAXBUFFER, "%H:%M:%S", timeinfo);
    snprintf(tline, MAXBUFFER, "cchatd version %.2f local time: %s", VERSION, title);
    strcpy(title, tline);
    telluser(connectionid, title, BLUE);
    telluser(connectionid, "----------------------------------------", BLUE);
    for (i1=0;i1<nchannels;i1++) {
     if ( channels[i1]->active == ON ) {
      sprintf( tline, "ch:%s%s%stopic:%s users:%d", channels[i1]->name, (channels[i1]->locked == 1 ) ? "[priv]" : "[pub]", (outconnections[i1]->fd > -1) ? "* " : " ",  channels[i1]->topic, channelusers(i1) );
      telluser(connectionid, tline, CYAN);
     }
    }
    snprintf(tline, MAXBUFFER, "--> online since %s <--", cchatddatetime);
    telluser(connectionid, tline, YELLOW);
   }
   if ( i == NAMES ) {
    if (nwords != 1) {
     telluser(connectionid, "<names> ->no parameters<-", RED);
     return 0;
    }
    listchannelusers(connectionid, 1);
   }
   if ( i == LOCK ) {
    if (nwords != 1) {
     telluser(connectionid, "<lock> ->no parameters<-", RED);
     return 0;
    }
    if ( connections[connectionid]->channel == CONSOLEID ) {
     telluser(connectionid, "channel zero cannot be locked!\n", RED);
     return 0;
    }
    if ( channels[connections[connectionid]->channel]->locked == 1 )
     telluser(connectionid, "channel is already locked!", RED);
    else {
     channels[connections[connectionid]->channel]->locked=1;
     telluser(connectionid, "channel locked", CYAN);
    }
   }
   if ( i == UNLOCK ) {
    if (nwords != 1) {
     telluser(connectionid, "<unlock> ->no parameters<-", RED);
     return 0;
    }
    if ( channels[connections[connectionid]->channel]->locked == 0 )
     telluser(connectionid, "channel is already unlocked!", RED);
    else {
     channels[connections[connectionid]->channel]->locked=0;
     telluser(connectionid, "channel unlocked", CYAN);
    }
   }
   if ( i == MSG ) {
    if ( nwords < 3 || (i2=whoisnick(words[1]))==-1 ) {
     telluser(connectionid, "<msg> nickname message", RED);
     return 0;
    }
    sprintf(tline, "%s[priv]:", connections[connectionid]->nickname);
    for (i1=2;i1<nwords;i1++) {
     strcat(tline, bwords[i1]);
     strcat(tline, " ");
    }
    strcat(tline, "");
    telluser(i2, tline, CYAN);
   }
   if ( i == LOG ) {
    if ( nwords != 2 ) {
     telluser(connectionid, "<log> level", RED);
     return 0;
    }
    if ( comms == 1 ) { // only on or off accepted
     i2=0;
     if ( !strcmp(words[1], STATES[OFF]) )
      i2=NONE ;
     if ( !strcmp(words[1], STATES[ON]) )
      i2=FULL;
    }
    if ( comms == 0 ) {
     if ( (i2=atoi(words[1]))==0 ) {
      for (i2=1;i2<5;i2++) 
       if ( !strcmp(words[1], LOGLEVELS[i2]) )
        break;
     }
    }
    if ( i2 > FULL || i2 < NONE || loglevel == i2 ) {
      if ( comms == 1 ) 
       strcpy(tline, "<log> on|off");
      else
       snprintf( tline, MAXBUFFER, "<log> <none|basic|increased|full|1..4> current: <%s>", LOGLEVELS[loglevel] );
      telluser(connectionid, tline, RED);
      return 0;
     }
     if ( loglevel == NONE && i2 > NONE )
      logfile=fopen(LOGFILE, "a");
     loglevel=i2;
     if ( comms == 1 ) 
      i2 = (loglevel == NONE ) ? OFF : ON;
     snprintf( tline, MAXBUFFER, "log level set to <%s>", ( comms == 1 ) ? STATES[i2] : LOGLEVELS[i2] );
     telluser(connectionid, tline, GREEN);
    }
    if ( i == REPORT ) {
     if ( nwords == 1 ) {
      telluser(connectionid, "<report> your text", RED);
      return 0;
     }
     telluser(connectionid, "report submitted", GREEN);
     if ( loglevel == NONE )
      return 0;
     snprintf(tline, MAXBUFFER, "%s has submitted a report: ", connections[connectionid]->nickname);
     for (i2=1;i2<nwords;i2++) {
      strcat(tline, words[nwords]);
      strcat(tline, " ");
     }
     strcat(tline, "");
     logaction(tline, BASIC);
    }
    if ( i == OPEN ) {
     if ( nwords != 3 ) {
      telluser(connectionid, "<open> server port", RED);
      return 0;
     }
     if ( outconnections[connections[connectionid]->channel]->fd > -1 ) {
      telluser(connectionid, "channel already connected", RED);
      return 0;
     }
     if ( (outconnections[connections[connectionid]->channel]->fd=clientsocket(words[1], words[2])) == -1 ) {
      return 0;
     }
     strcpy( outconnections[connections[connectionid]->channel]->ipaddress, words[1] );
     strcpy( outconnections[connections[connectionid]->channel]->port, words[2] );
     snprintf(tline, MAXBUFFER, "channel now connected to %s %s", outconnections[connections[connectionid]->channel]->ipaddress, outconnections[connections[connectionid]->channel]->port);
     if ( outconnections[connections[connectionid]->channel]->pipe == ON )
      tellchannelusers(tline, GREEN, connections[connectionid]->channel, OFF);
    }
    if ( i == CLOSE ) {
     if ( nwords != 1 ) {
      telluser(connectionid, "<close> ->no parameters<-", RED);
      return 0;
     }
     if ( outconnections[connections[connectionid]->channel]->fd == -1 ) {
      telluser(connectionid, "channel is not connected", RED);
      return 0;
     }
     closeoutconnection(connections[connectionid]->channel);
     if ( outconnections[connections[connectionid]->channel]->pipe == ON )
      tellchannelusers("connection closed/reset", GREEN, connections[connectionid]->channel, OFF);
    }
    if ( i == MUTESERVER ) {
     if ( nwords > 2 ) {
      telluser(connectionid, "<mute> or <mute> on|off", RED);
      return 0;
     }
     for (i2=0;i2<2;i2++)
      if ( !strcmp(STATES[i2], words[1]) )
       break;
     if  ( i2 == 2 )
      muteserver=(muteserver == OFF) ? ON : OFF;
     else
      muteserver=i2;
     snprintf(tline, MAXBUFFER, "cchatd mute server is %s", STATES[muteserver]);
     telluser(connectionid, tline, GREEN);
    }
    if ( i == PIPE ) {
     if ( nwords > 2 ) {
      telluser(connectionid, "<pipe> or <pipe> on|off", RED);
      return 0;
     }
     for (i2=0;i2<2;i2++)
      if ( !strcmp(STATES[i2], words[1]) )
       break;
     if  ( i2 == 2 )
      outconnections[connections[0]->channel]->pipe=(outconnections[connections[0]->channel]->pipe == ON) ? OFF : ON;
     else
      outconnections[connections[0]->channel]->pipe=i2;
     snprintf( tline, MAXBUFFER,  "data pipe for channel is now %s", STATES[outconnections[connections[0]->channel]->pipe] );
     tellchannelusers( tline, GREEN, connections[0]->channel, OFF );
    }
    if ( i == HIDE ) {
     if ( nwords > 2 ) {
      telluser(connectionid, "<hide> or <hide> on|off", RED);
      return 0;
     }
    for (i2=0;i2<2;i2++)
      if ( !strcmp(STATES[i2], words[1]) )
       break;
     if  ( i2 == 2 )
      connections[connectionid]->active=(connections[connectionid]->active == ON) ? HIDDEN : ON;
     else {
      i2=( i2 == ON ) ? HIDDEN : OFF;
      connections[connectionid]->active=i2;
     }
     snprintf(tline, MAXBUFFER, "you are now%shidden", (connections[connectionid]->active == HIDDEN) ? " " : " not ");
     telluser(connectionid, tline, GREEN);
     snprintf(tline, MAXBUFFER, "%s suddenly %s", connections[connectionid]->nickname, (connections[connectionid]->active == HIDDEN) ? "dissappears" : "appears");
     tellchannelusers( tline, GREEN, connections[connectionid]->channel, OFF );
    }
    if ( i == READ ) {
     if ( nwords != 3 ) {
      telluser(connectionid, "<read> nickname(or all) filename", RED);
      return 0;
     }
     if ( (i2=whoisnick(words[1])) == -1 && strcmp(words[1], EVERYBODY) ) {
      telluser(connectionid, "this user is not logged in", RED);
      return 0;
     }
     if ( i2 == -1 ) // everybody
      for (i1=0;i1<nconnections;i1++)
       outputtextfile(i1, words[2]);
     else
      outputtextfile(i2, words[2]);
    }
     
    
 return 1;
}

// join other channel
void joinchannel(int connectionid, int nextchannel)
{
  char tline[MAXBUFFER];
    
   if ( comms == 0 ) {
    sprintf(tline, "%s has left the channel", connections[connectionid]->nickname);
    tellchannelusers( tline, GREEN, connections[connectionid]->channel, OFF );
   }
   dismisschannel(connectionid);
   connections[connectionid]->channel=nextchannel;
   if ( comms == 0 ) {
    sprintf(tline, "%s has joined %s", connections[connectionid]->nickname, channels[nextchannel]->name);
    tellchannelusers( tline, GREEN, nextchannel, OFF );
    listchannelusers(connectionid, 0);   
   }
}
    
