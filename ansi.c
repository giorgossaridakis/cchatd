// ansi screen to ncurses
#include <stdio.h>
#include <string.h>
#include <ncurses.h>
#include <ctype.h>

// definitions
#define MAXDEFINITION 11
#define MAXNAME 50

const char *ANSISEQUENCES[] = { 
  "\e[0;30m", // regular text
  "\e[0;31m",
  "\e[0;32m",
  "\e[0;33m",
  "\e[0;34m",
  "\e[0;35m",
  "\e[0;36m",
  "\e[0;37m",
  "\e[1;30m", // 8 regular bold text
  "\e[1;31m",
  "\e[1;32m",
  "\e[1;33m",
  "\e[1;34m",
  "\e[1;35m",
  "\e[1;36m",
  "\e[1;37m",
  "\e[4;30m", // 16 regular underlined text
  "\e[4;31m",
  "\e[4;32m",
  "\e[4;33m",
  "\e[4;34m",
  "\e[4;35m",
  "\e[4;36m",
  "\e[4;37m",
  "\e[5;30m", // 24 regular blink text
  "\e[5;31m",
  "\e[5;32m",
  "\e[5;33m",
  "\e[5;34m",
  "\e[5;35m",
  "\e[5;36m",
  "\e[5;37m",
  "\e[7;30m", // 32 regular reverse text
  "\e[7;31m",
  "\e[7;32m",
  "\e[7;33m",
  "\e[7;34m",
  "\e[7;35m",
  "\e[7;36m",
  "\e[7;37m",
  "\e[8;30m", // 40 regular hidden text
  "\e[8;31m",
  "\e[8;32m",
  "\e[8;33m",
  "\e[8;34m",
  "\e[8;35m",
  "\e[8;36m",
  "\e[8;37m",
  "\e[30m", // 48 regular text
  "\e[31m",
  "\e[32m",
  "\e[33m",
  "\e[34m",
  "\e[35m",
  "\e[36m",
  "\e[37m",
  "\e[40m", // 56 background
  "\e[41m",
  "\e[42m",
  "\e[43m",
  "\e[44m",
  "\e[45m",
  "\e[46m",
  "\e[47m",
  "\e[0;100m", // 64 high intensty background
  "\e[0;101m",
  "\e[0;102m",
  "\e[0;103m",
  "\e[0;104m",
  "\e[0;105m",
  "\e[0;106m",
  "\e[0;107m",
  "\e[0;90m", // 72 high intensty text
  "\e[0;91m",
  "\e[0;92m",
  "\e[0;93m",
  "\e[0;94m",
  "\e[0;95m",
  "\e[0;96m",
  "\e[0;97m",
  "\e[1;90m", // 80 bold high intensity text
  "\e[1;91m",
  "\e[1;92m",
  "\e[1;93m",
  "\e[1;94m",
  "\e[1;95m",
  "\e[1;96m",
  "\e[1;97m",
  "\e[38;40;0m", // 88 old class reset
  "\e[0;0m", 
  "\e[0m", // 90 reset
  "\e[1m", // bold
  "\e[4m", // underlined
  "\e[5m", // blink
  "\e[7m", // reverse
  "\e[8m", // hidden

                    };
enum { ANSI_RESET = 90, ANSI_BOLD, ANSI_UNDERLINE, ANSI_BLINK, ANSI_REVERSE, ANSI_HIDDEN };
enum { ANSI_COLOR_BLACK = 0 , ANSI_COLOR_RED, ANSI_COLOR_GREEN, ANSI_COLOR_YELLOW, ANSI_COLOR_BLUE, ANSI_COLOR_MAGENTA, ANSI_COLOR_CYAN, ANSI_COLOR_WHITE };
enum { ANSI_BACKGROUND_BLACK = 56, ANSI_BACKGROUND_RED, ANSI_BACKGROUND_GREEN, ANSI_BACKGROUND_YELLOW, ANSI_BACKGROUND_BLUE, ANSI_BACKGROUND_MAGENTA, ANSI_BACKGROUND_CYAN, ANSI_BACKGROUND_WHITE };
enum { BLACKBACKGROUND = 56, REDBACKGROUMD, GREENBACKGROUND, YELLOWBACKGROUMD, BLUEBACKGROUND, MAGENTABACKGROUND, CYANBACKGROUND, WHITEBACKGROUND };
#define OLDCLASSRESET 64
enum { CRESET = 90, BOLDON, UNDERLINEON, BLINKON, REVERSEON, HIDDENON };

// enum { NORMAL=0, STANDOUT, UNDERLINE, REVERSE, BLINK, DIM, BOLD, PROTECT, INVISIBLE };
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
      
int COLORPAIRS[8][2] = { {    BLACKONBLACK,    ANSI_COLOR_BLACK    },
                         {    REDONBLACK,      ANSI_COLOR_RED      },
                         {    GREENONBLACK,    ANSI_COLOR_GREEN    },
                         {    YELLOWONBLACK,   ANSI_COLOR_YELLOW   },
                         {    BLUEONBLACK,     ANSI_COLOR_BLUE     },
                         {    MAGENTAONBLACK,  ANSI_COLOR_MAGENTA  },
                         {    CYANONBLACK,     ANSI_COLOR_CYAN     },
                         {    WHITEONBLACK,    ANSI_COLOR_WHITE    }
                       };

typedef struct {
    int pair;
    int attribute;
               } NCursesDefinition;
    
NCursesDefinition ncursesdefinitions[]= {
                                      {      BLACKONBLACK,      A_NORMAL      }, 
                                      {      REDONBLACK,        A_NORMAL      },
                                      {      GREENONBLACK,      A_NORMAL      },
                                      {      YELLOWONBLACK,     A_NORMAL      },
                                      {      BLUEONBLACK,       A_NORMAL      },
                                      {      MAGENTAONBLACK,    A_NORMAL      },
                                      {      CYANONBLACK,       A_NORMAL      },
                                      {      WHITEONBLACK,      A_NORMAL      },
                                      {      BLACKONBLACK,      A_BOLD        },
                                      {      REDONBLACK,        A_BOLD        },
                                      {      GREENONBLACK,      A_BOLD        },
                                      {      YELLOWONBLACK,     A_BOLD        },
                                      {      BLUEONBLACK,       A_BOLD        },
                                      {      MAGENTAONBLACK,    A_BOLD        },
                                      {      CYANONBLACK,       A_BOLD        },
                                      {      WHITEONBLACK,      A_BOLD        },
                                      {      BLACKONBLACK,      A_UNDERLINE   },
                                      {      REDONBLACK,        A_UNDERLINE   },
                                      {      GREENONBLACK,      A_UNDERLINE   },
                                      {      YELLOWONBLACK,     A_UNDERLINE   },
                                      {      BLUEONBLACK,       A_UNDERLINE   },
                                      {      MAGENTAONBLACK,    A_UNDERLINE   },
                                      {      CYANONBLACK,       A_UNDERLINE   },
                                      {      WHITEONBLACK,      A_UNDERLINE   },
                                      {      BLACKONBLACK,      A_BLINK       },
                                      {      REDONBLACK,        A_BLINK       },
                                      {      GREENONBLACK,      A_BLINK       },
                                      {      YELLOWONBLACK,     A_BLINK       },
                                      {      BLUEONBLACK,       A_BLINK       },
                                      {      MAGENTAONBLACK,    A_BLINK       },
                                      {      CYANONBLACK,       A_BLINK       },
                                      {      WHITEONBLACK,      A_BLINK       },
                                      {      BLACKONBLACK,      A_REVERSE     },
                                      {      REDONBLACK,        A_REVERSE     },
                                      {      GREENONBLACK,      A_REVERSE     },
                                      {      YELLOWONBLACK,     A_REVERSE     },
                                      {      BLUEONBLACK,       A_REVERSE     },
                                      {      MAGENTAONBLACK,    A_REVERSE     },
                                      {      CYANONBLACK,       A_REVERSE     },
                                      {      WHITEONBLACK,      A_REVERSE     },                         
                                      {      BLACKONBLACK,      A_INVIS       },
                                      {      REDONBLACK,        A_INVIS       },
                                      {      GREENONBLACK,      A_INVIS       },
                                      {      YELLOWONBLACK,     A_INVIS       },
                                      {      BLUEONBLACK,       A_INVIS       },
                                      {      MAGENTAONBLACK,    A_INVIS       },
                                      {      CYANONBLACK,       A_INVIS       },
                                      {      WHITEONBLACK,      A_INVIS       }                              
                                    };
                                
// functions
NCursesDefinition* filteransisequence(char sequence[MAXDEFINITION]);
int snatchansitextpart(int startpt, char *text, char part[MAXDEFINITION]);
int isansisequencechar(char t);
// extern
extern void writeterminal(char *text, int c);
               
// filter ansi sequence
NCursesDefinition* filteransisequence(char sequence[MAXDEFINITION])
{
  static NCursesDefinition ncursesdefinition;
  int i;
  static NCursesDefinition backupdefinition;
  
   // modes
   if ( !strcmp(sequence, ANSISEQUENCES[OLDCLASSRESET]) || !strcmp(sequence, ANSISEQUENCES[OLDCLASSRESET+1]))
    strcpy(sequence, ANSISEQUENCES[CRESET]);
   if ( strlen(sequence) == 4 ) {
    for (i=CRESET;i<HIDDENON+1;i++) {
     if ( !strcmp(sequence, ANSISEQUENCES[i]) ) {
       switch ( i ) {
        case CRESET:
         ncursesdefinition.attribute=A_NORMAL;
         ncursesdefinition.pair=WHITEONBLACK;
        break;
        case BOLDON:
         ncursesdefinition.attribute=backupdefinition.attribute | A_BOLD;
         ncursesdefinition.pair=backupdefinition.pair;
        break;
        case UNDERLINEON:
         ncursesdefinition.attribute=backupdefinition.attribute | A_UNDERLINE;
         ncursesdefinition.pair=backupdefinition.pair;
        break;
        case BLINKON:
         ncursesdefinition.attribute=backupdefinition.attribute | A_BLINK;
         ncursesdefinition.pair=backupdefinition.pair;
        break;            
        case REVERSEON:
         ncursesdefinition.attribute=backupdefinition.attribute | A_REVERSE;
         ncursesdefinition.pair=backupdefinition.pair;
        break;
        case HIDDENON:
         ncursesdefinition.attribute=backupdefinition.attribute | A_INVIS;
         ncursesdefinition.pair=backupdefinition.pair;
        break;
       }
      break;
     }
    }
   }
  
   // transform regular colors to first definitions
   for (i=48;i<56;i++) {
    if ( !strcmp(sequence, ANSISEQUENCES[i]) ) {
     strcpy(sequence, ANSISEQUENCES[i-48]);
     break;
    }
   }
    
   // transform high intensity sequences
   for (i=64;i<72;i++) {
    if ( !strcmp(sequence, ANSISEQUENCES[i]) ) {
     strcpy(sequence, ANSISEQUENCES[i-8]);
     break;
    }
   }
   for (i=72;i<88;i++) {
    if ( !strcmp(sequence, ANSISEQUENCES[i]) ) {
     strcpy(sequence, ANSISEQUENCES[i-56]);
     break;
    }
   }
   
   // change text
   if ( strlen(sequence) == 7 ) {
    for (i=0;i<48;i++) {
     if ( !strcmp(sequence, ANSISEQUENCES[i]) )
      break;
    }
    if ( i < 48 ) {
     ncursesdefinition.attribute=ncursesdefinitions[i].attribute;
     ncursesdefinition.pair=ncursesdefinitions[i].pair;
    }
   }
   // change background
   if ( strlen(sequence) == 5 ) {
    // read current attributes
    ncursesdefinition.attribute=backupdefinition.attribute; // do not add attribute
    // retrieve definition
    for (i=BLACKBACKGROUND;i<WHITEBACKGROUND+1;i++)
     if ( !strcmp(sequence, ANSISEQUENCES[i]) )
      break;
    if ( i == WHITEBACKGROUND+1 )
     return &ncursesdefinition;

    switch ( backupdefinition.pair ) {
     case BLACKONBLACK:
      if ( i == BLACKBACKGROUND )
       ncursesdefinition.pair=BLACKONBLACK;
      if ( i == REDBACKGROUMD )
       ncursesdefinition.pair=BLACKONRED;
      if ( i == GREENBACKGROUND )
       ncursesdefinition.pair=BLACKONGREEN;
      if ( i == YELLOWBACKGROUMD )
       ncursesdefinition.pair=BLACKONYELLOW;
      if ( i == BLUEBACKGROUND )
       ncursesdefinition.pair=BLACKONBLUE;
      if ( i == MAGENTABACKGROUND )
       ncursesdefinition.pair=BLACKONMAGENTA;
      if ( i == CYANBACKGROUND )
       ncursesdefinition.pair=BLACKONCYAN;
      if ( i == WHITEBACKGROUND )
       ncursesdefinition.pair=BLACKONWHITE;
     break;
     case REDONBLACK:
      if ( i == BLACKBACKGROUND )
       ncursesdefinition.pair=REDONBLACK;
      if ( i == REDBACKGROUMD )
       ncursesdefinition.pair=REDONRED;
      if ( i == GREENBACKGROUND )
       ncursesdefinition.pair=REDONGREEN;
      if ( i == YELLOWBACKGROUMD )
       ncursesdefinition.pair=REDONYELLOW;
      if ( i == BLUEBACKGROUND )
       ncursesdefinition.pair=REDONBLUE;
      if ( i == MAGENTABACKGROUND )
       ncursesdefinition.pair=REDONMAGENTA;
      if ( i == CYANBACKGROUND )
       ncursesdefinition.pair=REDONCYAN;
      if ( i == WHITEBACKGROUND )
       ncursesdefinition.pair=REDONWHITE;
     break;
     case GREENONBLACK:
       if ( i == BLACKBACKGROUND )
       ncursesdefinition.pair=GREENONBLACK;
      if ( i == REDBACKGROUMD )
       ncursesdefinition.pair=GREENONRED;
      if ( i == GREENBACKGROUND )
       ncursesdefinition.pair=GREENONGREEN;
      if ( i == YELLOWBACKGROUMD )
       ncursesdefinition.pair=GREENONYELLOW;
      if ( i == BLUEBACKGROUND )
       ncursesdefinition.pair=GREENONBLUE;
      if ( i == MAGENTABACKGROUND )
       ncursesdefinition.pair=GREENONMAGENTA;
      if ( i == CYANBACKGROUND )
       ncursesdefinition.pair=GREENONCYAN;
      if ( i == WHITEBACKGROUND )
       ncursesdefinition.pair=GREENONWHITE;
     break;
     case YELLOWONBLACK:
       if ( i == BLACKBACKGROUND )
       ncursesdefinition.pair=YELLOWONBLACK;
      if ( i == REDBACKGROUMD )
       ncursesdefinition.pair=YELLOWONRED;
      if ( i == GREENBACKGROUND )
       ncursesdefinition.pair=YELLOWONGREEN;
      if ( i == YELLOWBACKGROUMD )
       ncursesdefinition.pair=YELLOWONYELLOW;
      if ( i == BLUEBACKGROUND )
       ncursesdefinition.pair=YELLOWONBLUE;
      if ( i == MAGENTABACKGROUND )
       ncursesdefinition.pair=YELLOWONMAGENTA;
      if ( i == CYANBACKGROUND )
       ncursesdefinition.pair=YELLOWONCYAN;
      if ( i == WHITEBACKGROUND )
       ncursesdefinition.pair=YELLOWONWHITE;
     break;
     case BLUEONBLACK:
       if ( i == BLACKBACKGROUND )
       ncursesdefinition.pair=BLUEONBLACK;
      if ( i == REDBACKGROUMD )
       ncursesdefinition.pair=BLUEONRED;
      if ( i == GREENBACKGROUND )
       ncursesdefinition.pair=BLUEONGREEN;
      if ( i == YELLOWBACKGROUMD )
       ncursesdefinition.pair=BLUEONYELLOW;
      if ( i == BLUEBACKGROUND )
       ncursesdefinition.pair=BLUEONBLUE;
      if ( i == MAGENTABACKGROUND )
       ncursesdefinition.pair=BLUEONMAGENTA;
      if ( i == CYANBACKGROUND )
       ncursesdefinition.pair=BLUEONCYAN;
      if ( i == WHITEBACKGROUND )
       ncursesdefinition.pair=BLUEONWHITE;
     break;
     case CYANONBLACK:
       if ( i == BLACKBACKGROUND )
       ncursesdefinition.pair=CYANONBLACK;
      if ( i == REDBACKGROUMD )
       ncursesdefinition.pair=CYANONRED;
      if ( i == GREENBACKGROUND )
       ncursesdefinition.pair=CYANONGREEN;
      if ( i == YELLOWBACKGROUMD )
       ncursesdefinition.pair=CYANONYELLOW;
      if ( i == BLUEBACKGROUND )
       ncursesdefinition.pair=CYANONBLUE;
      if ( i == MAGENTABACKGROUND )
       ncursesdefinition.pair=CYANONMAGENTA;
      if ( i == CYANBACKGROUND )
       ncursesdefinition.pair=CYANONCYAN;
      if ( i == WHITEBACKGROUND )
       ncursesdefinition.pair=CYANONWHITE;
     break;
     case WHITEONBLACK:
       if ( i == BLACKBACKGROUND )
       ncursesdefinition.pair=WHITEONBLACK;
      if ( i == REDBACKGROUMD )
       ncursesdefinition.pair=WHITEONRED;
      if ( i == GREENBACKGROUND )
       ncursesdefinition.pair=WHITEONGREEN;
      if ( i == YELLOWBACKGROUMD )
       ncursesdefinition.pair=WHITEONYELLOW;
      if ( i == BLUEBACKGROUND )
       ncursesdefinition.pair=WHITEONBLUE;
      if ( i == MAGENTABACKGROUND )
       ncursesdefinition.pair=WHITEONMAGENTA;
      if ( i == CYANBACKGROUND )
       ncursesdefinition.pair=WHITEONCYAN;
      if ( i == WHITEBACKGROUND )
       ncursesdefinition.pair=WHITEONWHITE;
     break;
    }

   }
   backupdefinition=ncursesdefinition;
   
 return &ncursesdefinition;
} 

enum { NOTANSI=0, ANSI, ANSIEND };
// copy part of string, until char
int snatchansitextpart(int startpt, char *text, char part[MAXDEFINITION])
{
  int i=startpt, i1=0, filter;
    
   memset(part, 0, MAXDEFINITION);
   part[i1++]=text[i++];
   while ( (filter=isansisequencechar(text[i])) > 0 && i1 < MAXDEFINITION - 1 ) {
    part[i1++]=text[i++];
    if ( filter == ANSIEND )
     break;
   }
   part[i1]='\0';
    
 return i1-1;   
}

// is char 
int isansisequencechar(char t)
{
  char ANSISEQUENCECHARS[] = { '[', ';', 's', 'u', 'h', 'J', 'K' };
  int i;
  
   if ( isdigit(t) )
    return ANSI;
   for (i=0;i<7;i++)
    if ( t == ANSISEQUENCECHARS[i] )
     return ANSI;  
   if ( t == 'm' || t == 'z' || t == 'h' )
    return ANSIEND;
        
 return NOTANSI;
}
