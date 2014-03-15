
# comma separated list (e.g. "iconv.dll,libiconv.dll")
DEFAULT_LIBICONV_DLL ?= \"\"


prefix ?=/usr/local/mingw
INSTALL ?=install

COPY ?=cp
MKDIR ?=mkdir
SVN_EXPORT ?=svn export
ZIP ?=zip

CFLAGS += -pedantic -Wall
CPPFLAGS += -DUSE_LIBICONV_DLL
CPPFLAGS += -DDEFAULT_LIBICONV_DLL=$(DEFAULT_LIBICONV_DLL)

DLLWRAP ?=dllwrap
RANLIB ?=ranlib
STRIP ?=strip

all: iconv.dll libiconv.a win_iconv.exe

dist: test win_iconv.zip

iconv.dll: win_iconv.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -DMAKE_DLL
	$(DLLWRAP) --dllname $@ --def iconv.def win_iconv.o $(SPECS_FLAGS)
	$(STRIP) $@

libiconv.a: win_iconv.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<
	$(AR) rcs $@ win_iconv.o
	$(RANLIB) $@

win_iconv.exe: win_iconv.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -s -o$@ $< -DMAKE_EXE

libmlang.a: mlang.def
	$(DLLTOOL) --kill-at --input-def $< --output-lib $@

test:
	$(CC) $(CPPFLAGS) $(CFLAGS) -s -o win_iconv_test.exe win_iconv_test.c
	./win_iconv_test.exe

win_iconv.zip: msvcrt msvcr70 msvcr71
	$(RM) -r win_iconv
	$(SVN_EXPORT) . win_iconv
	$(COPY) msvcrt/iconv.dll msvcrt/win_iconv.exe win_iconv/
	$(MKDIR) win_iconv/msvcr70
	$(COPY) msvcr70/iconv.dll win_iconv/msvcr70/
	$(MKDIR) win_iconv/msvcr71
	$(COPY) msvcr71/iconv.dll win_iconv/msvcr71/
	$(ZIP) -r $@ win_iconv

msvcrt:
	$(SVN_EXPORT) . $@
	$(MAKE) -C $@

msvcr70:
	$(SVN_EXPORT) . $@
	$(CC) -dumpspecs | sed s/-lmsvcrt/-l$@/ > $@/specs
	$(MAKE) -C $@ "SPECS_FLAGS=-specs=$$PWD/specs"

msvcr71:
	$(SVN_EXPORT) . $@
	$(CC) -dumpspecs | sed s/-lmsvcrt/-l$@/ > $@/specs
	$(MAKE) -C $@ "SPECS_FLAGS=-specs=$$PWD/specs"

install: all
	-$(MKDIR) -p $(DESTDIR)$(prefix)/bin
	$(INSTALL) iconv.dll $(DESTDIR)$(prefix)/bin/iconv.dll
	$(INSTALL) win_iconv.exe $(DESTDIR)$(prefix)/bin/win_iconv.exe
	-$(MKDIR) -p $(DESTDIR)$(prefix)/lib
	$(INSTALL) libiconv.a  $(DESTDIR)$(prefix)/lib/libiconv.a
	-$(MKDIR) -p $(DESTDIR)$(prefix)/include
	$(INSTALL) iconv.h  $(DESTDIR)$(prefix)/include/iconv.h

clean:
	$(RM) win_iconv.exe
	$(RM) win_iconv.o
	$(RM) iconv.dll*
	$(RM) libiconv.a
	$(RM) libiconv.dll
	$(RM) win_iconv_test.exe
	$(RM) libmlang.a
	$(RM) -r win_iconv
	$(RM) -r win_iconv.zip
	$(RM) -r msvcrt
	$(RM) -r msvcr70
	$(RM) -r msvcr71
