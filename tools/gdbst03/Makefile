#
# Makefile for GDB Stub for DJGPP/Mingw 0.3 source distribution
#
# Copyright 2000 by Jonathan Brogdon
#

include Makefile.cfg

default:
	@$(ECHO) Welcome to GDB Stub for DJGPP & Mingw 0.3 source distribution!
	@$(ECHO) To make the GDB stub type:
	@$(ECHO) 	make all		- Make library and demo programs
	@$(ECHO) 	make library		- Make only library
	@$(ECHO) 	make demo		- Make demo program
	@$(ECHO) 	make install		- Install library and header files
	@$(ECHO) 	make uninstall		- Uninstall library and header files
	@$(ECHO) 	make clean		- Remove .o files
	@$(ECHO) 	make distclean		- Remove ready binaries and .o files
	@$(ECHO) 	make dep		- Make dependences

# Inform make of phony targets
.PHONY:	library demo clean blankdep dep distclean install

all: dep library demo
	@$(ECHO) Library and demo program created

./lib/libgdbst.a:
	@mkdir -p lib
	@make -s -C ./src/library all

library: ./lib/libgdbst.a
	@$(ECHO) Library created

demo: ./lib/libgdbst.a
	@mkdir -p demo
	@make -s -C ./src/demo all
	@$(ECHO) Demo program created

clean:
	@make -s -C ./src/library clean
	@make -s -C ./src/demo clean
	@$(ECHO) Clean complete

blankdep:
# Create blank depend.dep files to avoid errors
	@$(ECHOBLANK) > ./src/library/depend.dep
	@$(ECHOBLANK) > ./src/demo/depend.dep

# Now carry on as usual
dep:	blankdep
	@make -s -C ./src/library dep
	@make -s -C ./src/demo dep
	@$(ECHO) Created dependency files

# Blank all the dependencies too
distclean:	blankdep
	@make -s -C ./src/library distclean
	@make -s -C ./src/demo distclean
	@$(RM) -r ./demo/
	@$(RM) -r ./lib/
	@$(ECHO) Cleaned up files

install: library
ifdef DJGPP
	@cp lib/*.a $(DJDIR)/lib
	@ginstall -d $(DJDIR)/include
	@cp include/*.h $(DJDIR)/include
	@$(ECHO) GDB Stub Library for DJGPP installed
endif

uninstall:
ifdef DJGPP
	@$(RM) $(DJDIR)/lib/libgdb.a
	@$(RM) $(DJDIR)/include/i386-stub.h
	@$(ECHO) GDB Stub Library for DJGPP uninstalled
ifdef DJGPP







