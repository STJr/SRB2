.PHONY : all clean

all : flatb

flatb.exe : flatb.c
	i686-w64-mingw32-gcc $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(LDLIBS) -o $@ $<

clean :
	$(RM) flatb flatb.exe
