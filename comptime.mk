#Add-on Makefile for wxDev-C++ project file
SRCDIR=src
ifdef ComSpec
COMSPEC=$(ComSpec)
endif

all-before:
ifdef COMSPEC
	${RM} $(SRCDIR)\comptime.h
	comptime.bat $(SRCDIR)
else
	${RM} $(SRCDIR)/comptime.h
	./comptime.sh $(SRCDIR)
endif

clean-custom:
ifdef COMSPEC
	${RM} $(SRCDIR)\comptime.h
else
	${RM} $(SRCDIR)/comptime.h
endif
