ifdef UNIXCOMMON
LUA_CFLAGS+=-DLUA_USE_POSIX
endif
ifdef LINUX
LUA_CFLAGS+=-DLUA_USE_POSIX
endif
ifdef GCC43
ifndef GCC44
WFLAGS+=-Wno-logical-op
endif
endif

OBJS:=$(OBJS) \
	$(OBJDIR)/lapi.o \
	$(OBJDIR)/lbaselib.o \
	$(OBJDIR)/ldo.o \
	$(OBJDIR)/lfunc.o \
	$(OBJDIR)/linit.o \
	$(OBJDIR)/liolib.o \
	$(OBJDIR)/llex.o \
	$(OBJDIR)/lmem.o \
	$(OBJDIR)/lobject.o \
	$(OBJDIR)/lstate.o \
	$(OBJDIR)/lstrlib.o \
	$(OBJDIR)/ltablib.o \
	$(OBJDIR)/lundump.o \
	$(OBJDIR)/lzio.o \
	$(OBJDIR)/lauxlib.o \
	$(OBJDIR)/lcode.o \
	$(OBJDIR)/ldebug.o \
	$(OBJDIR)/ldump.o \
	$(OBJDIR)/lgc.o \
	$(OBJDIR)/lopcodes.o \
	$(OBJDIR)/lparser.o \
	$(OBJDIR)/lstring.o \
	$(OBJDIR)/ltable.o \
	$(OBJDIR)/ltm.o \
	$(OBJDIR)/lvm.o \
	$(OBJDIR)/lua_script.o \
	$(OBJDIR)/lua_baselib.o \
	$(OBJDIR)/lua_mathlib.o \
	$(OBJDIR)/lua_hooklib.o \
	$(OBJDIR)/lua_consolelib.o \
	$(OBJDIR)/lua_infolib.o \
	$(OBJDIR)/lua_mobjlib.o \
	$(OBJDIR)/lua_playerlib.o \
	$(OBJDIR)/lua_skinlib.o \
	$(OBJDIR)/lua_thinkerlib.o \
	$(OBJDIR)/lua_maplib.o \
	$(OBJDIR)/lua_blockmaplib.o \
	$(OBJDIR)/lua_hudlib.o
