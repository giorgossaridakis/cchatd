#   cchatd Makefile with Macros
#   - Author: Giorgos Saridakis

# definitions
CC      = $(shell which cc)
CFLAGS  =  -Wall
LDLIBS  =  -lncurses
# list of sources
SOURCES = screen.c ansi.c command.c socket.c socklib.c parser.c cchatd.c
EXEC    = cchatd

#convert to a list of objects
OBJECTS = $(SOURCES:.c=.o)

$(EXEC): $(OBJECTS)
	 $(CC) -o$@ $(SOURCES) $(CFLAGS) $(LDLIBS)
	 
clean:
	rm -f $(OBJECTS)
