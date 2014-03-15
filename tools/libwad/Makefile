#!/usr/bin/env make -f

## libwad: Doom WAD format interface library.
## Copyright (C) 2011 by Callum Dickinson.
#
## This program is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License
## as published by the Free Software Foundation; either version 2
## of the License, or (at your option) any later version.
#
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.

# Library name.
LIBNAME = wad
# Library version.
LIBVERM = 1
LIBVER	= $(LIBVERM).0.0

# Library default options.
CPPFLAGS	+= -DLIBNAME="$(LIBNAME)" -DLIBVER="$(LIBVER)"

# Source files and directory.
SRCDIR	= src
SRC	= $(SRCDIR)/wad.c $(SRCDIR)/lump.c $(SRCDIR)/wad_static.c

# Compiled files directory.
BINDIR	= bin
INCDIR	= include

#
## Variables that shouldn't be edited unless
## you know what you are doing start here.
#

# Makefile pretty text output variables.
PRAR     	= AR      
PRCC     	= CC      
PRINSTALL	= INSTALL 
PRCLEAN  	= CLEAN   

# Library name prefix.
LIBPFX	= lib

# Library installation directory.
PREFIX	?= /usr/local
LIBDIR	:= $(PREFIX)/lib
INCLUDE := $(PREFIX)/include

# Variables created during compilation.
OBJ	= $(SRC:.c=.o)
DEP	= $(BINDIR)/$(LIBPFX)$(LIBNAME).d
ifdef MINGW
DEFN	= $(LIBNAME).def
LIBN	= $(LIBPFX)$(LIBNAME).lib
BIN	= $(BINDIR)/$(LIBPFX)$(LIBNAME)-$(LIBVERM).dll
else
SONAME	= $(LIBPFX)$(LIBNAME).so.$(LIBVERM)
BIN	= $(BINDIR)/$(LIBPFX)$(LIBNAME).so.$(LIBVER)
endif
STABIN	= $(BINDIR)/$(LIBPFX)$(LIBNAME).a

# Compilation programs.
ECHO	:= echo
RM	:= $(RM) -r
FIND	:= find
GREP	:= grep
INSTALL	:= install -m 644
MKDIR	:= mkdir -p
FOR	:= for
SED	:= sed
AR	:= $(CROSS_COMPILE)$(AR)
CC	:= $(CROSS_COMPILE)$(CC)

# Shell programs.
SHECHO   	:= $(ECHO)
SHFIND   	:= $(FIND)
SHGREP   	:= $(GREP)
SHINSTALL	:= $(INSTALL)
SHSED    	:= $(SED)
SHCC     	:= $(CC)

# Quiet compiling programs during pretty text output.
ECHO    	:= @$(ECHO)
ifndef V
RM      	:= @$(RM)
LDCONFIG	:= @$(LDCONFIG)
FIND    	:= @$(FIND)
GREP    	:= @$(GREP)
INSTALL 	:= @$(INSTALL)
MKDIR   	:= @$(MKDIR)
FOR     	:= @$(FOR)
SED     	:= @$(SED)
AR      	:= @$(AR)
CC      	:= @$(CC)
endif

LIBS	= $(BIN) $(STABIN)
HEADERS	= $(shell $(SHFIND) $(CURDIR) | $(SHGREP) -e "$(INCDIR)/.*.h" | $(SHGREP) -v ".svn" | $(SHSED) "s:$(CURDIR)/::")

# Compiler default flags.
ifndef MINGW
CFLAGS	+= -fPIC
LDFLAGS	+= -fPIC
endif
ifdef DEBUG
CFLAGS	+= -g $(M5) -O0 -Wall
endif

# Linker flags.
LDFLAGS	+= -shared
ifdef MINGW
LDFLAGS += -Wl,--output-def,$(DEFN),--out-implib,$(LIBN)
else
LDFLAGS += -Wl,-soname,$(SONAME)
endif

#
## Makefile targets.
#

# all is dependent on $(BIN) to be complete
all: $(DEP) $(BIN) $(STABIN)

# $(EXE) is dependent on all of the files in $(OBJ) to exist
$(DEP): $(SRC)
ifndef V
	$(ECHO) "  $(PRCC)$@"
endif
	-$(MKDIR) $(BINDIR) > /dev/null 2>&1
	-$(RM) $@
	$(FOR) f in $(SRC); do \
		$(SHCC) $(CPPFLAGS) $(CFLAGS) -MM $$f >> $@; \
	done
-include $@

%.o: %.c
ifndef V
	$(ECHO) "  $(PRCC)$@"
endif
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BIN): $(OBJ)
ifndef V
	$(ECHO) "  $(PRCC)$@"
endif
	$(CC) $(LDFLAGS) -o $@ $(OBJ) $(LIB)

$(STABIN): $(OBJ)
ifndef V
	$(ECHO) "  $(PRAR)$@"
endif
	$(AR) rcs $@ $(OBJ)

install: all install-libs install-headers

install-libs:
	$(MKDIR) $(LIBDIR)
ifndef V
	$(FOR) f in $(LIBS); do \
		$(SHECHO) "  $(PRINSTALL)$$f"; \
		$(SHINSTALL) $$f $(LIBDIR); \
	done
else
	$(FOR) f in $(LIBS); do \
		$(SHINSTALL) $$f $(LIBDIR); \
	done
endif
	$(LDCONFIG)

install-headers:
	$(MKDIR) $(INCLUDE)
ifndef V
	$(FOR) f in $(HEADERS); do \
		$(SHECHO) "  $(PRINSTALL)$$f"; \
		$(SHINSTALL) $$f $(INCLUDE); \
	done
else
	$(FOR) f in $(HEADERS); do \
		$(SHINSTALL) $$f $(INCLUDE); \
	done
endif

clean:
ifndef V
	$(ECHO) "  $(PRCLEAN)$(OBJ) $(LIBS)"
endif
	-$(RM) $(OBJ) $(LIBS)

distclean:
ifndef V
	$(ECHO) "  $(PRCLEAN)$(BINDIR)"
endif
	-$(RM) $(BINDIR)


.PHONY : all install install-libs install-headers clean
