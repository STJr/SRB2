# Makfile of SRB2MP

CFLAGS +=-Wall -mms-bitfields -fno-exceptions
LDFLAGS +=-mwindows -lfmod
WINDRESFLAGS=

SRC=lump.c SRB2MP.c

ifdef DEBUGMODE
CFLAGS +=-g -D_DEBUG
LDFLAGS +=-g
else
CFLAGS :=-Os -s $(CFLAGS)
LDFLAGS :=-s $(LDFLAGS)
endif

OBJ=$(SRC:.c=.o) # replaces the .c from SRC with .o
EXTRAOBJ=Script1.res
EXE=SRB2MP.exe

ifdef PREFIX
CC=$(PREFIX)-gcc
WINDRES ?=$(PREFIX)-windres
endif

WINDRES ?=windres

RM=rm

%.o: %.c StdAfx.h lump.h resource.h
	$(CC) $(CFLAGS) -o $@ -c $<

%.res: %.rc resource.h
	$(WINDRES) -i $< -O rc $(WINDRESFLAGS) -o $@ -O coff

.PHONY : all     # .PHONY ignores files named all
all: $(EXE)      # all is dependent on $(EXE) to be complete

$(EXE): $(OBJ) $(EXTRAOBJ) # $(EXE) is dependent on all of the files in $(OBJ) to exist
	$(CC) $(OBJ) $(EXTRAOBJ) $(LDFLAGS) -o $@

.PHONY : clean   # .PHONY ignores files named clean
clean:
	-$(RM) $(OBJ) $(EXTRAOBJ)
