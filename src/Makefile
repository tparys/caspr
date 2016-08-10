# Variables 
CC = gcc
CFLAGS = -I. -O2 -Wall
FILENAME = caspr
OBJECTS = main.o scan.o scanutil.o asmrec.o asmgen.o asmout.o symtab.o directive.o
MAINHEADERS = scan.h asm.h symtab.h global.h directive.h

# Rules

all: $(FILENAME)

debug: $(FILENAME)

$(FILENAME): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(FILENAME)
#	strip $(FILENAME)

%.o: %.c $(MAINHEADERS) Makefile
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(FILENAME)

run: all
	./$(FILENAME)
