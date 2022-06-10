// cchatd, multi chat server and client with ncurses
#include "cchatd.h"

const double VERSION=0.93;

int main(int argc, char *argv[])
{
  int i, i1, i2, fd, c, nbufflines, whoisonfd;

  char bufflines[MAXLINES][MAXBUFFER/2];
  struct timeval timeout;
  // initialize the timeout data structure
  timeout.tv_sec = 0;
  timeout.tv_usec = 125;
  
  signal(SIGINT, INThandler);
  
   initscreen();
   initvariables();
   comms=initcomms();
   if ( comms == 1 )
    loglevel=NONE;
   else
    logfile=fopen(LOGFILE, "a");
   wtimeout(win1, 50); // unblock ncurses stdin
   mousemask(0, NULL);
   // wait until terminal window becomes 80x24
   struct winsize w;
   ioctl( STDOUT_FILENO, TIOCGWINSZ, &w );
   while (w.ws_row != 24 || w.ws_col != 80) {
    gotoxy(1, 1);
    printw("terminal size %dx%d, cchatd requires 80x24               ", w.ws_col, w.ws_row);
    refresh();
    sleep(1);
    ioctl( STDOUT_FILENO, TIOCGWINSZ, &w );
   }
   clear();

   // start chatd
   if ( comms == 0 )
    snprintf(title, MAXINPUT, "*cchatd %.2f listening on %s port %d*", VERSION,  serveraddress, serverport);
   if ( comms == 1 ) // comms error
    snprintf(title, MAXINPUT, "*cchatd %.2f telnet client*", VERSION);   
   logaction(title, BASIC);
   if ( argc > 1 ) {
    if ( argc == 3 )
     outconnections[CONSOLEID].fd=clientsocket(argv[1], argv[2]);
    else
     writeterminal("usage: cchatd <server> port", RED);
   }
   cursor();
   if ( comms == 0 )
    outputtextfile(CONSOLEID, (char*)NEWSFILE);
   while ( true ) {
       
    showscreen();
    read_set = set;
    select(fd_hwm + 1, &read_set, NULL, NULL, &timeout); // last argument timeout value
   
    // read outbound sockets
    for (i=0;i<MAXCHANNELS;i++) {
     if ( outconnections[i].fd > -1 ) {
      if ( (nread=recv(outconnections[i].fd, clientbuffer, MAXBUFFER, 0)) > 0 ) {
       for (i1=0;i1<MAXLINES;i1++)
        memset(outconnections[i].bufferlines[i1], 0, MAXBUFFER/2);
       outconnections[i].nbuffers=stringformattersplitter(clientbuffer, outconnections[i].bufferlines);
       for (i1=0;i1<outconnections[i].nbuffers;i1++) {
        if ( connections[CONSOLEID].channel == i ) {
         writeterminal(outconnections[i].bufferlines[i1], WHITE);
        }
        if ( outconnections[i].pipe == ON )
         tellchannelusers(outconnections[i].bufferlines[i1], WHITE, i, ON);
        strcat(outconnections[i].bufferlines[i1], "\r\n");
        logoutconnectionaction(outconnections[i].bufferlines[i1], FULL);
       }
       if ( terminalnl == OFF ) {
        screen.x=screen.bx; --screen.y; // backup one row
       }
      }
      if ( nread == 0 )
       closeoutconnection(i);
     }
     memset(clientbuffer, 0, MAXBUFFER);
    }
    
    // read terminal keyboard
    if ( (c=readkeyboard(win1)) != ERR && c != 0) {
     strcpy(keyboardhistory[keyboardhistorypos++], buf);
     if ( keephistory == ON ) {
      keyboardhistory[keyboardhistorypos]=malloc(MAXINPUT);
      keyboardhistorysize++;
      i1=(keyboardhistory[keyboardhistorypos] == 0) ? keyboardhistorysize/5 : MAXHISTORY/5;
      if ( keyboardhistorysize > MAXHISTORY - 2 || keyboardhistory[keyboardhistorypos] == 0 )
       freekeyboardhistory(i1);
     }
     if ( (fdcommands(buf, CONSOLEID) > 0) )
      continue;
     if ( comms == 1 ) // telnet client only
      strcpy(connections[CONSOLEID].buffer, buf);
     else
      snprintf(connections[CONSOLEID].buffer, MAXBUFFER, "[ %s ] %s", (connections[0].active == HIDDEN) ?  HIDDENNAME : connections[0].nickname, buf);
     strcat(buf, "\r\n");
     logaction(buf, FULL); 
     if ( outconnections[connections[CONSOLEID].channel].fd > -1 ) {
      send( outconnections[connections[CONSOLEID].channel].fd, buf, strlen(buf), 0 );
     }
     if ( !strcmp(buf, "\r\n") )
      continue;  // skip empty line from here
     if ( outconnections[connections[CONSOLEID].channel].fd > -1 )
      writeterminal(buf, YELLOW);
     else
      writeterminal(connections[CONSOLEID].buffer, YELLOW);
     // write to all open connections
     for (i=1;i<MAXCONNECTIONS;i++) { // 0 is reserved for console
      if ( connections[i].active == ON && connections[i].channel == connections[CONSOLEID].channel ) {
       telluser(i, connections[CONSOLEID].buffer, WHITE);
      }
     }
    memset(buf, 0, MAXBUFFER/2);
    }
    
    // read listening sockets
    for (fd = 0; fd <= fd_hwm; fd++) {
     if (FD_ISSET(fd, &read_set)) {
      if ( fd == fd_skt ) {  // new connection
       if ( (whoisonfd=addconnection()) == 0 )
        continue;
       listchannelusers(whoisonfd, 0);
      }
      else {
       if ( (whoisonfd=whoisfd(fd)) == -1 )
        continue;
       memset( connections[whoisonfd].buffer, 0, CONNECTIONBUFFER );
       memset( buf, 0, MAXBUFFER/2 );
       nread = read( fd, buf, MAXBUFFER/2 );
       if ( nread == 0 )
        disconnect(fd);
       if ( nread ) {
        nbufflines=stringformattersplitter(buf, bufflines);
        for (i2=0;i2<nbufflines;i2++) {
         if ( (fdcommands(bufflines[i2], whoisonfd) > 0) )
          continue;
         // send to outbound socket, if any
         if ( outconnections[connections[whoisonfd].channel].pipe == ON && outconnections[connections[whoisonfd].channel].fd > - 1 )
          send(outconnections[connections[whoisonfd].channel].fd, buf, strlen(buf), 0);         
         snprintf(connections[whoisonfd].buffer, CONNECTIONBUFFER, "[ %s ] %s\r\n", (connections[whoisonfd].active == HIDDEN) ?  HIDDENNAME : connections[whoisonfd].nickname, bufflines[i2]);
         if ( connections[whoisonfd].channel == connections[0].channel )
          writeterminal(connections[whoisonfd].buffer, WHITE);
         logaction(connections[whoisonfd].buffer, FULL);
         for (i1=1;i1<MAXCONNECTIONS;i1++) { // 0 is reserved for console
          if ( connections[i1].active == 1 && whoisonfd != i1 && connections[whoisonfd].channel == connections[i1].channel ) {
           write( connections[i1].fd, connections[whoisonfd].buffer, strlen(connections[whoisonfd].buffer) );
          }
         }
        }
        if ( terminalnl == OFF ) {
         screen.x=screen.bx; --screen.y; // backup one row
        }
        memset(buf, 0, MAXBUFFER/2);
        memset(connections[whoisonfd].buffer, 0, CONNECTIONBUFFER);
       }
      }
     }
    }
   
   }

return 0;
}

// initialize variables
void initvariables()
{
 int i, i1;
 
    serveraddress=ipaddress();
    serverport=SERVERPORT;
    // prepare connections
    for (i=1;i<MAXCONNECTIONS;i++) {
     connections[i].active=OFF;
     connections[i].fd=-1;
    }
    connections[0].fd=STDOUT_FILENO;
    connections[0].active=ON;
    // console
    strcpy( connections[0].ipaddress, serveraddress );
    connections[0].port=serverport;
    strcpy( connections[0].nickname, CONSOLE );
    connections[0].operator=ON;
    connections[0].channel=OFF;
    connections[0].invitation=OFF;
    connections[0].active=ON;
  
    // channels
    for (i=0;i<MAXCHANNELS;i++) {
     channels[i].active=OFF;
     channels[i].locked=OFF;
     sprintf( channels[i].name, "%d", i ); 
     outconnections[i].fd=-1;
     outconnections[i].pipe=OFF;
    }
    // channel zero
    strcpy( channels[0].name, MAINHALL );
    strcpy( channels[0].topic, MAINHALLTOPIC );
    channels[CONSOLEID].active=ON;
  
    // screen
     myrow=24; mycol=1; screen.x=1; screen.y=2;
     for (i=1;i<SCREENX;i++) {
      for (i1=1;i1<SCREENY;i1++) {
       screen.attributes[i][i1]=NORMAL;
       screen.colors[i][i1]=BLACK;
       screen.characters[i][i1]=SPACE;
      }
     }
     // keyboard buffer
     keyboardhistory[keyboardhistorypos]=malloc(MAXINPUT);
     // read port
     if ( (i1=open(PORTFILE, O_RDONLY)) == -1 )
      return;
     char tport[MAXNAME];
     if ( (readfileentry(i1, tport)) == 0 )
      return;
     close(i1);
     if ( (i=atoi(tport)) < 1000 )
      return;
     serverport=i;
}

// read ncurses keyboard
int readkeyboard(WINDOW *twin)
{
//   static int bufpos, i;
  int i, i1, c;
  char tbuf[MAXBUFFER/2];

    if ( (c=wgetch(twin)) != ERR ) {
        
      // previous screens
     if ( keephistory == ON ) {
      if ( c == PGUP || c == PGDN ) {
       if ( c == PGUP && screenspos > 0 && screenssize > 0 ) { // screen up  
        --screenspos;
        previousscreen=ON;
       }
       if ( c == PGDN && screenspos < screenssize ) { // screen down  
        ++screenspos;
        previousscreen=ON;
        if ( screenspos == screenssize ) {
         previousscreen=OFF;
         displayscreen(&screen);
        }
       }
      }
      if ( previousscreen == ON ) {
       displayscreen(screens[screenspos]);
       if ( c == PGUP || c == PGDN )
        return 0;
       previousscreen=OFF;
       displayscreen(&screen);
      }
     }
        
     // handle control keys
     if ( c == TAB ) { // next channel
      if ( comms == 1 ) {
       if ( connections[CONSOLEID].channel < MAXCHANNELS - 2 )
        channels[connections[CONSOLEID].channel + 1].active=ON;
       if ( connections[CONSOLEID].channel != CONSOLEID && outconnections[connections[CONSOLEID].channel].fd == -1 )
        channels[connections[CONSOLEID].channel].active=OFF;
      }
      for (i=(connections[CONSOLEID].channel < MAXCHANNELS-2) ? connections[CONSOLEID].channel+1 : CONSOLEID;i!=connections[CONSOLEID].channel;i++) {
       if ( i == MAXCHANNELS )
        i=CONSOLEID;
       if ( channels[i].active == ON )
        break;
      }
      if ( i != connections[CONSOLEID].channel ) {
       joinchannel(CONSOLEID, i);
       return 0;
      }
     }
     if ( c == SHIFT_TAB ) { // previous channel
      if ( comms == 1 ) { 
       if ( connections[CONSOLEID].channel > CONSOLEID + 1 )
        channels[connections[CONSOLEID].channel - 1].active=ON;
       if ( connections[CONSOLEID].channel == CONSOLEID )
        channels[MAXCHANNELS-1].active=ON;
       if ( connections[CONSOLEID].channel != CONSOLEID && outconnections[connections[CONSOLEID].channel].fd == -1 )
        channels[connections[CONSOLEID].channel].active=OFF;
      }
      for (i=(connections[CONSOLEID].channel > CONSOLEID) ? connections[CONSOLEID].channel-1 : MAXCHANNELS-1;i!=connections[CONSOLEID].channel;i--) {
       if ( i == -1 )
        i=MAXCHANNELS-1;
       if ( channels[i].active == ON )
        break;
      }
      if ( i != connections[CONSOLEID].channel ) {
       joinchannel(CONSOLEID, i);
       return 0;
      }
     }
     if ( c == UP || c == DOWN ) {
      if ( c == UP && keyboardhistorypos > 0 ) { // history up
       if ( keyboardhistorypos == keyboardhistorysize ) {
        buf[bufpos]='\0';
        strcpy(keyboardhistory[keyboardhistorysize], buf);
       }
       strcpy(buf, keyboardhistory[--keyboardhistorypos]);
      }
      if ( c == DOWN && keyboardhistorypos < keyboardhistorysize ) // history down
       strcpy(buf, keyboardhistory[++keyboardhistorypos]);
      if ( !strcmp(buf, "\r\n") )
       memset(buf, 0, MAXBUFFER/2);
      clearinputline();
      gotoxy(1, myrow);
      color(YELLOW);
      printw("%s", buf);
      refresh();
      mycol=bufpos=strlen(buf); if ( mycol < MAXINPUT) mycol++;
      return 0;
     }
     if ( (c == BACKSPACE || c == BACKSPACE2) && mycol > 1 ) {
      i1=0;
      for (i=0;i<mycol-2;i++)
       tbuf[i1++]=buf[i];
      for (++i;i<bufpos;i++)
       tbuf[i1++]=buf[i];
      bufpos--;
      mycol--;
      for (i=0;i<bufpos;i++)
       buf[i]=tbuf[i];
     }
     if ( c == HOME )
      mycol=1;
     if ( c == END )
      mycol=bufpos+1;
     
     if ( c == INSERT && mycol < bufpos && bufpos < MAXINPUT ) {
      i1=0;
      for (i=0;i<bufpos;i++) {
       if ( i == mycol - 1 )
        tbuf[i1++]=SPACE;
       tbuf[i1++]=buf[i];
      }
      ++bufpos;
       for (i=0;i<bufpos && i < MAXINPUT;i++)
        buf[i]=tbuf[i];
      }
     if ( c == DELETE && mycol < bufpos + 1 ) {
      for (i=mycol-1;i<MAXINPUT-1;i++)
       buf[i]=buf[i+1];
      --bufpos;
     }
     if ( c == LEFT && mycol > 1 )
      --mycol;
     if ( c == RIGHT && mycol < bufpos + 1 )
      if ( mycol < MAXINPUT )
       ++mycol;
         
     if ( isprintablecharacter(c) && bufpos < MAXINPUT + 1 ) {
      buf[mycol-1]=(char) c;
      if ( mycol - 1 == bufpos )
       ++bufpos;
      if ( mycol < MAXINPUT )
       ++mycol;
     }
     clearinputline();
     color(YELLOW);
     gotoxy(1, myrow);
     for (i=0;i<bufpos;i++)
      addch(buf[i]);
     gotoxy(mycol, myrow);
     refresh();
     if ( c == '\n' ) {
      mycol=1; keyboardhistorypos=keyboardhistorysize;
      clearinputline();
      buf[bufpos]='\0';
      bufpos=0;
     }
    
    }
      
 return ( c == '\n' ) ? c : 0;
}

// clear lines in keyboard history
void freekeyboardhistory(int lines)
{
  int i, tkeyboardhistorypos=0;
  char* tkeyboardhistory[MAXHISTORY];
  
   for (i=lines;i<MAXHISTORY;i++)
    tkeyboardhistory[tkeyboardhistorypos++]=keyboardhistory[i];
   for (i=0;i<MAXHISTORY-lines;i++)
    keyboardhistory[i]=tkeyboardhistory[i];
   for (;i<MAXHISTORY;i++)
    memset(keyboardhistory[i], 0, MAXINPUT);
   keyboardhistorypos=keyboardhistorysize=tkeyboardhistorypos;
   keyboardhistory[keyboardhistorypos]=malloc(MAXINPUT);
   if ( keyboardhistory[keyboardhistorypos] == 0 )
    keephistory=OFF;
}

// log requested text
ui logaction(char *text, int level)
{
 char title[MAXNAME];  
 char ttext[strlen(text)];
 strcpy(ttext, text);
  
  if ( level > loglevel || loglevel == NONE )
   return 0;
  if ( logfile == NULL )
    return 0;
   
   time (&rawtime);
   timeinfo = localtime (&rawtime);
   strftime(title, MAXNAME, "[%c] ", timeinfo);
   if ( text[strlen(ttext)-1] != '\n' )
    strcat(ttext, "\r\n");
   fprintf(logfile, "%s %s", title, ttext);
   fflush(logfile);
   
 return loglevel;
}

// log requested text
ui logoutconnectionaction(char *text, int level)
{
  char ttext[strlen(text)];
  strcpy(ttext, text);
  
  if ( level > loglevel )
   return 0;
  if ( logfile == 0 )
    return 0;

   if ( text[strlen(ttext)-1] != '\n' )
    strcat(ttext, "\n");
   fprintf(logfile, "%s", ttext);
   fflush(logfile);
   
 return loglevel;
}


// read text file to connection
void outputtextfile(int connectionid, char *filename)
{
  FILE *stream;
  char *line = NULL;
  size_t len = 0;
  ssize_t nread;
  if ( connections[connectionid].active == 0 )
   return;
  
     stream = fopen(filename, "r");
     if ( stream == NULL )
      return;

     while ( (nread = getline(&line, &len, stream)) != -1 ) {
      if ( connectionid == 0 )
       writeterminal(line, WHITE);
      else
       write( connections[connectionid].fd, line, nread );
      memset(line, 0, nread);
     }
    
    fclose(stream);
}

// end chatd
void endcchatd()
{
 int i;
 char tline[MAXINPUT];
 
  sprintf(tline, "cchatd %s is closing down", ( comms == 0 ) ? "server" : "client");
  announce(tline, GREEN);
  logaction("cchatd closing", BASIC);
  for (i=1;i<MAXCONNECTIONS;i++) { // 0 is console
   if ( connections[i].active == 1 ) {
    disconnect(connections[i].fd);
   }
  }
  for (i=0;i<MAXCHANNELS;i++) {
   if ( outconnections[i].fd > -1 ) {
    close(outconnections[i].fd);
   }
  }
   
   close(fd_skt);
   shutdown(fd_skt, 2);
   if ( logfile )
    fclose(logfile);
   endscreen();
   exit(EXIT_SUCCESS);
}

// INT signal catcher
void INThandler(int sig)
{
   signal(sig, SIG_IGN);
   printf("ctrl+c detected\r\n");
   logaction("break from keyboard", INCREASED);
   endcchatd();
}

