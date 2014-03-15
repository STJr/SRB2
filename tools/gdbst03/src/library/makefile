#
# Makefile for GDB Stub for DJGPP
#
# GDB Stub for DJGPP Copyright 2000 by Jonathan Brogdon
#

include ../../Makefile.cfg

CFLAGS += -I../../include -I../include

LOBJS =	i386-stub.o i386-supp.o

all: library

library:	$(LOBJS)
	@$(RM) ../../lib/libgdbst.a
	@$(AR) rcs ../../lib/libgdbst.a	$(LOBJS)

dep:
	@$(CC) $(CFLAGS) -M *.c > depend.dep

clean:
	@$(RM) $(LOBJS)

distclean: clean	
	@$(RM) ../../lib/libgdbst.a
	@$(RM) depend.dep

$(OBJS) $(LOBJS):
include depend.dep
