#!/usr/bin/env make -f

## wadexample: libwad example program.
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

# Program name.
PRONAME = wadexample
# Program version.
PROVER	= 1.0.0

# Program default options.
CPPFLAGS	+= -DPRONAME="$(PRONAME)" -DPROVER="$(PROVER)"

# Source files and directory.
SRC	= wadexample.c

# Required libraries.
LIB	= -lwad

#
## Variables that shouldn't be edited unless
## you know what you are doing start here.
#

# Makefile pretty text output variables.
PRCC	= CC      
PRINSTALL=INSTALL 
PRCLEAN	= CLEAN   

# Library name prefix.
LIBPFX	= lib

# Library installation directory.
PREFIX	?= /usr/local
LIBDIR	:= $(PREFIX)/lib
INCLUDE := $(PREFIX)/include

# Variables created during compilation.
OBJ	= $(SRC:.c=.o)
DEP	= $(PRONAME).d
ifdef MINGW
BIN	= $(PRONAME).exe
else
BIN	= $(PRONAME)
endif

# Compilation programs.
ECHO	:= echo
RM	:= $(RM) -r
FIND	:= find
GREP	:= grep
INSTALLX:= install -m 755
MKDIR	:= mkdir -p
FOR	:= for
SED	:= sed
CC	:= $(CROSS_COMPILE)$(CC)

# Shell programs.
SHECHO	:= $(ECHO)
SHFIND	:= $(FIND)
SHGREP	:= $(GREP)
SHINSTALL:= $(INSTALL)
SHSED	:= $(SED)
SHCC	:= $(CC)

# Quiet compiling programs during pretty text output.
NULL	:= /dev/null 2>&1
ECHO	:= @$(ECHO)
ifndef V
RM	:= @$(RM)
LDCONFIG:= @$(LDCONFIG)
FIND	:= @$(FIND)
GREP	:= @$(GREP)
INSTALL	:= @$(INSTALL)
MKDIR	:= @$(MKDIR)
FOR	:= @$(FOR)
SED	:= @$(SED)
CC	:= @$(CC)
endif

# Compiler default flags.
ifdef DEBUG
CFLAGS	+= -g -O0 -Wall -W
endif

# Linker flags.
#LDFLAGS +=

#
## Makefile targets.
#

all: $(DEP) $(BIN)

$(DEP): $(SRC)
ifndef V
	$(ECHO) "  $(PRCC)$@"
endif
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

install: all
ifndef V
	$(SHECHO) "  $(PRINSTALL)$(BIN)"
endif
	$(INSTALLX) $(BIN) $(INSTALLDIR)

clean:
ifndef V
	$(ECHO) "  $(PRCLEAN)$(OBJ) $(DEP)"
endif
	-$(RM) $(OBJ) $(DEP)

.PHONY : all install clean
