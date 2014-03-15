#
# Makefile for GDB Stub demo
# Written by Jonathan Brogodn
#
# GDB Stub for DJGPP Copyright 2000 by Jonathan Brogdon
#

include ../../Makefile.cfg

CFLAGS += -g
CFLAGS += -I../../include -I../include -I.
CFLAGS += -DDEBUG_COM_PORT=1
CFLAGS += -DDEBUG_COM_PORT_SPEED=9600
CFLAGS += -DREMOTE_DEBUGGING

# Objects to build
OBJS =	serdbg.o crc_16.o

all:	demo

demo:	$(OBJS)
ifdef DJGPP
	@$(LD) $(LDFLAGS) -Map ./$@.map -o../../demo/$@.exe $(DJDIR)/lib/crt0.o $(OBJS) -L$(DJDIR)/lib -L../../lib -lgdbst -ldzcom -lc -lgcc
endif
ifdef WINDOWS
	@$(LD) $(LDFLAGS) -Map ./$@.map -o../../demo/$@.exe $(DJDIR)/lib/crt0.o $(OBJS) -L../../lib -lgdbst -lwsock32 -lc -lgcc
endif

clean:
	@$(RM) $(OBJS)
	@$(RM) *.map

distclean: clean	
	@$(RM) $(OBJS)
	@$(RM) depend.dep
	@$(RM) ../../demo/*.exe

dep:
	@$(CC) $(CFLAGS) -M *.c > depend.dep

$(OBJS):
include depend.dep
