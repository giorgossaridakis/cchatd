// cchatd sockets
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
#include <ncurses.h>

#define MAXBUFFER 5120
enum { NONE = 1, BASIC, INCREASED, FULL }; // log levels
enum { CONTROL=1, FCNTL, REUSE, BIND, LISTEN, ADDRESSLOOKUP, CONNECTION };
const char *ERRORS[] = { "creating control socket", "fcntl on control socket", "socket reuse error", "port bind error", "socket listen error", "address lookup error", "connection error" };
typedef unsigned int ui;

struct sockaddr_in sa, sa2;
socklen_t sa2_len = sizeof sa2;
extern fd_set set, read_set;
extern int serverport, fd_skt, fd_hwm;
extern int BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE;

// function declarations
//local
int initcomms();
int clientsocket(char* host, char* port);
char *ipaddress();
// external
extern ui logaction(char *text, int level);
extern void writeterminal(char *text, int c);

// initialize communications
int initcomms()
{
  int i, arg, perror=0;
  struct termios options;
 
   // unblock stdin
   fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
   tcgetattr( STDIN_FILENO, &options); // read current options
   options.c_lflag &= ~ICANON; // disable canonical line oriented input
   options.c_lflag &= ~ECHOCTL; // do not echo control chars as ^char, delete as ~?
   tcsetattr( STDIN_FILENO, TCSANOW, &options ); // write back to stdin
    
   // server socket setup
 
   // settings
   sa.sin_family = AF_INET;
   sa.sin_port = htons(serverport);
   sa.sin_addr.s_addr = htonl(INADDR_ANY);
   
   // open socket
   if ( (fd_skt = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
    perror=CONTROL;
   
   if ( perror == 0 ) {
    // make sure socket doesn't block
    if (fcntl(fd_skt, F_SETFL, FNDELAY)==-1)
     perror=FCNTL;
   }
   
   if ( perror == 0 ) {
    // allow address reuse 
    if ((setsockopt(fd_skt, SOL_SOCKET, SO_REUSEADDR, (char *) &i, sizeof i))==-1)
     perror=REUSE;
   }
   
   if ( perror == 0 ) {
    // bind por
    if ((i=bind(fd_skt, (struct sockaddr *)&sa, sizeof(sa)))==-1)
     perror=BIND;
   }
   
   if ( perror == 0 ) {  
    if ((i=listen(fd_skt, SOMAXCONN))==-1)
     perror=LISTEN;
   }
   
   if ( perror > 0 ) {
    return 1;
   }
   
   // zero fd set and add socket
   FD_ZERO(&set);
   FD_SET(fd_skt, &set);
   if (fd_skt > fd_hwm)
    fd_hwm = fd_skt;
   // unblock socket
   arg= fcntl(fd_skt, F_GETFL, NULL);
   arg |= O_NONBLOCK;
   arg |= O_NDELAY;
   fcntl(fd_skt, F_SETFL, arg);
   
 return 0;   
}

// TCP/IP telnet socket 
int clientsocket(char* host, char* port)
{
   int rc, s, arg, perror=0;
   char tline[MAXBUFFER];
   struct addrinfo hints, *res;
   
   // client socket setup
   memset(&hints, 0, sizeof(hints));

   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = AI_PASSIVE;

   if ((rc = getaddrinfo(host, port, &hints, &res) ) < 0 )
    perror=ADDRESSLOOKUP;
   
   if ( perror == 0 ) {
   s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if ( s < 0 )
     perror=CONTROL;
   }

   if ( perror == 0 ) {
    if ( connect(s, res->ai_addr, res->ai_addrlen) < 0 )
     perror=CONNECTION;
   }
   
   if ( perror > 0 ) {
    snprintf( tline, MAXBUFFER, "<connect> error: %s", ERRORS[perror-1] );
    logaction( tline, BASIC);
    writeterminal( tline, RED );
    return -1;
   }
   
   // unblock socket
  arg= fcntl(s, F_GETFL, NULL);
  arg |= O_NONBLOCK;
  arg |= O_NDELAY;
  fcntl(s, F_SETFL, arg);
  
   freeaddrinfo(res);
   
 return s;
}

// read IP address
char *ipaddress()
{
  struct ifaddrs * ifAddrStruct = NULL, * ifa = NULL;
  void * tmpAddrPtr = NULL;
  static char addressBuffer[INET_ADDRSTRLEN];

   getifaddrs(&ifAddrStruct);
   for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
     if (ifa ->ifa_addr->sa_family == AF_INET) { // Check it is IPv4
      char mask[INET_ADDRSTRLEN];
      void* mask_ptr = &((struct sockaddr_in*) ifa->ifa_netmask)->sin_addr;
      inet_ntop(AF_INET, mask_ptr, mask, INET_ADDRSTRLEN);
      if (strcmp(mask, "255.0.0.0") != 0) {
       // Is a valid IPv4 Address
       tmpAddrPtr = &((struct sockaddr_in *) ifa->ifa_addr)->sin_addr;
    inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN); } } }
    if (ifAddrStruct != NULL)
        freeifaddrs(ifAddrStruct);
 
 return &addressBuffer[0];
}
